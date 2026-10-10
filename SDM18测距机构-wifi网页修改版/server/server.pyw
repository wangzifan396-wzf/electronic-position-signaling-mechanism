"""
SDM18 钻具控制系统 - Web 控制面板服务器
双击此文件启动，自动打开浏览器。
纯 Python 标准库，无需 pip install 任何东西。
"""
import asyncio
import json
import os
import re
import socket
import sys
import time
import webbrowser
from datetime import datetime

TCP_PORT = 8080
HTTP_PORT = 3000
THIS_DIR = os.path.dirname(os.path.abspath(__file__))

# ═══════════════════════════════════════════════════════════════
#  Local IP detection
# ═══════════════════════════════════════════════════════════════
def get_local_ip():
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        s.connect(('8.8.8.8', 80))
        ip = s.getsockname()[0]
        s.close()
        return ip
    except Exception:
        for name in socket.gethostbyname_ex(socket.gethostname())[2]:
            if not name.startswith('127.'):
                return name
        return '127.0.0.1'

LOCAL_IP = get_local_ip()

# ═══════════════════════════════════════════════════════════════
#  Ring buffer
# ═══════════════════════════════════════════════════════════════
class RingBuffer:
    def __init__(self, max_size):
        self.max = max_size
        self.data = [None] * max_size
        self.head = 0
        self.count = 0

    def push(self, item):
        self.data[self.head] = item
        self.head = (self.head + 1) % self.max
        if self.count < self.max:
            self.count += 1

    def to_list(self):
        if self.count == 0:
            return []
        start = 0 if self.count < self.max else self.head
        result = []
        for i in range(self.count):
            result.append(self.data[(start + i) % self.max])
        return result

    def clear(self):
        self.head = 0
        self.count = 0

# ═══════════════════════════════════════════════════════════════
#  Protocol parser
# ═══════════════════════════════════════════════════════════════
def parse_line(line):
    if not line:
        return None

    # Telemetry: DATA|t|dist|mode|status|in_place|rev|avg|diff|t_in|t_gnd|spd
    if line.startswith('DATA|'):
        fields = line.split('|')
        if len(fields) < 12:
            return None
        try:
            return {
                'type': 'telemetry',
                't': int(fields[1]) if fields[1] else 0,
                'dist': int(fields[2]) if fields[2] else 0,
                'mode': fields[3] or 'UNKNOWN',
                'status': fields[4] or 'UNKNOWN',
                'in_place': 1 if fields[5] == '1' else 0,
                'rev': 1 if fields[6] == '1' else 0,
                'avg': int(fields[7]) if fields[7] else 0,
                'diff': int(fields[8]) if fields[8] else 0,
                't_in': int(fields[9]) if fields[9] else 0,
                't_gnd': int(fields[10]) if fields[10] else 0,
                'spd': int(fields[11]) if fields[11] else 0,
            }
        except (ValueError, IndexError):
            return None

    # Alerts
    if line.startswith('!!!') or line.startswith('***'):
        return {'type': 'event', 'text': line}

    # Responses
    if line.startswith('OK|') or line.startswith('ERROR|'):
        return {'type': 'response', 'text': line}

    # PARAMS response
    if line.startswith('PARAMS|'):
        params = {}
        parts = line.split('|')
        for i in range(1, len(parts)):
            kv = parts[i].split('=', 1)
            if len(kv) == 2:
                try:
                    k = kv[0].lower()
                    v = int(kv[1])
                    key_map = {'in_place': 'in_place', 'ground': 'ground',
                               'full': 'full', 'stuck': 'stuck', 'speed': 'speed'}
                    if k in key_map:
                        params[key_map[k]] = v
                except ValueError:
                    pass
        return {'type': 'params_response', 'text': line, 'params': params}

    return None

# ═══════════════════════════════════════════════════════════════
#  Shared state
# ═══════════════════════════════════════════════════════════════
class StateManager:
    def __init__(self):
        self.stm32_connected = False
        self.latest_telemetry = None
        self.current_params = {
            'in_place': 360, 'ground': 700, 'full': 70,
            'stuck': 2, 'speed': 100
        }
        self.event_log = RingBuffer(500)
        self.distance_history = RingBuffer(600)
        self.start_time = time.time()
        self._sse_queues = []  # list of asyncio.Queue for SSE clients

    def add_sse_queue(self, q):
        self._sse_queues.append(q)

    def remove_sse_queue(self, q):
        try:
            self._sse_queues.remove(q)
        except ValueError:
            pass

    async def _broadcast(self, event_type, data):
        payload = f"event: {event_type}\ndata: {json.dumps(data, ensure_ascii=False)}\n\n"
        dead = []
        for q in self._sse_queues:
            try:
                await q.put(payload)
            except Exception:
                dead.append(q)
        for q in dead:
            self.remove_sse_queue(q)

    async def update_telemetry(self, telemetry):
        self.latest_telemetry = telemetry
        self.distance_history.push({
            'time': time.time() * 1000,
            'dist': telemetry['dist'],
            'avg': telemetry['avg'],
            'mode': telemetry['mode'],
            'status': telemetry['status']
        })
        t = dict(telemetry)
        t['_point'] = self.distance_history.data[
            (self.distance_history.head - 1) % self.distance_history.max
        ] if self.distance_history.count > 0 else {'time': time.time() * 1000}
        await self._broadcast('telemetry', t)

    async def add_event(self, text):
        entry = {'time': time.time() * 1000, 'text': text}
        self.event_log.push(entry)
        await self._broadcast('event', entry)

    async def update_connection(self, connected):
        if self.stm32_connected != connected:
            self.stm32_connected = connected
            if not connected:
                self.latest_telemetry = None
            await self._broadcast('connection', {'connected': connected})

    async def update_params(self, params):
        self.current_params.update(params)
        await self._broadcast('params', dict(self.current_params))

    async def send_snapshot(self, q):
        snap = {
            'stm32Connected': self.stm32_connected,
            'latestTelemetry': self.latest_telemetry,
            'currentParams': dict(self.current_params),
            'eventLog': self.event_log.to_list(),
            'distanceHistory': self.distance_history.to_list(),
            'uptime': int(time.time() - self.start_time),
            'serverIP': LOCAL_IP,
            'tcpPort': TCP_PORT,
        }
        payload = f"event: snapshot\ndata: {json.dumps(snap, ensure_ascii=False)}\n\n"
        await q.put(payload)


# ═══════════════════════════════════════════════════════════════
#  TCP Bridge (async)
# ═══════════════════════════════════════════════════════════════
class TcpBridge:
    def __init__(self, state):
        self.state = state
        self.stm_writer = None
        self.stm_addr = None
        self.observers = {}  # writer -> addr

    async def handle_client(self, reader, writer):
        addr = writer.get_extra_info('peername')
        remote = f"{addr[0]}:{addr[1]}"
        print(f"[TCP] 新连接: {remote}")
        buffer = ''
        self.observers[writer] = remote

        try:
            while True:
                data = await reader.read(4096)
                if not data:
                    break

                buffer += data.decode('utf-8', errors='replace')
                lines = buffer.split('\n')
                buffer = lines.pop() or ''

                for raw_line in lines:
                    line = raw_line.rstrip('\r').strip()
                    if not line:
                        continue

                    parsed = parse_line(line)

                    if parsed and parsed['type'] == 'telemetry':
                        if writer != self.stm_writer:
                            if self.stm_writer:
                                print(f"[TCP] STM32 重分配 (旧连接已断)")
                            self.stm_writer = writer
                            self.stm_addr = remote
                            await self.state.update_connection(True)
                            print(f"[TCP] STM32 已识别: {remote}")
                        await self.state.update_telemetry(parsed)
                        await self._relay(line + '\r\n', exclude=writer)

                    elif parsed and parsed['type'] in ('event', 'response', 'params_response'):
                        if parsed['type'] == 'event' or parsed['type'] == 'response':
                            await self.state.add_event(parsed['text'])
                        elif parsed['type'] == 'params_response':
                            await self.state.add_event(parsed['text'])
                            if parsed.get('params'):
                                await self.state.update_params(parsed['params'])
                        await self._relay(line + '\r\n', exclude=writer)

                    else:
                        # From a non-STM32 socket → treat as command
                        if writer != self.stm_writer:
                            print(f"[TCP] 来自观察者 {remote} 的指令: {line}")
                            await self.send_to_stm(line + '\n')
                        await self._relay(line + '\r\n', exclude=writer)

        except asyncio.CancelledError:
            pass
        except Exception as e:
            print(f"[TCP] {remote} 错误: {e}")
        finally:
            print(f"[TCP] 断开: {remote}")
            if writer == self.stm_writer:
                self.stm_writer = None
                self.stm_addr = None
                await self.state.update_connection(False)
                print("[TCP] STM32 已断开")
            self.observers.pop(writer, None)
            try:
                writer.close()
            except Exception:
                pass

    async def _relay(self, data, exclude=None):
        dead = []
        for w in self.observers:
            if w is exclude or w is self.stm_writer:
                continue
            try:
                w.write(data.encode('utf-8'))
                await w.drain()
            except Exception:
                dead.append(w)
        for w in dead:
            self.observers.pop(w, None)

    async def send_to_stm(self, line):
        if not line.endswith('\n'):
            line += '\n'
        if self.stm_writer:
            try:
                self.stm_writer.write(line.encode('utf-8'))
                await self.stm_writer.drain()
                await self.state.add_event(f">> {line.strip()}")
                return True
            except Exception:
                pass
        return False

    async def start(self):
        server = await asyncio.start_server(
            self.handle_client, '0.0.0.0', TCP_PORT)
        print(f"[TCP] 桥接服务运行在端口 {TCP_PORT}")
        print(f"[TCP] ESP8266 应连接: {LOCAL_IP}:{TCP_PORT}")
        return server


# ═══════════════════════════════════════════════════════════════
#  HTTP Server (async) — serves index.html, SSE, POST /cmd
# ═══════════════════════════════════════════════════════════════
INDEX_PATH = os.path.join(THIS_DIR, 'public', 'index.html')

async def http_handler(reader, writer, state):
    try:
        raw = await asyncio.wait_for(reader.readuntil(b'\r\n\r\n'), timeout=30)
    except (asyncio.TimeoutError, asyncio.IncompleteReadError):
        writer.close()
        return

    request = raw.decode('utf-8', errors='replace')
    lines = request.split('\r\n')
    if not lines:
        writer.close()
        return

    first_line = lines[0]
    parts = first_line.split(' ')
    method = parts[0] if len(parts) > 0 else 'GET'
    path = parts[1] if len(parts) > 1 else '/'

    # Parse headers
    headers = {}
    for line in lines[1:]:
        if ':' in line:
            key, val = line.split(':', 1)
            headers[key.strip().lower()] = val.strip()

    content_length = int(headers.get('content-length', 0))

    # Read body if present
    body = ''
    if content_length > 0:
        try:
            body_raw = await asyncio.wait_for(reader.readexactly(content_length), timeout=10)
            body = body_raw.decode('utf-8', errors='replace')
        except Exception:
            body = ''

    # ── Route ────────────────────────────────────────────────
    if method == 'GET' and (path == '/' or path == '/index.html'):
        await serve_file(writer, INDEX_PATH, 'text/html; charset=utf-8')

    elif method == 'GET' and path == '/events':
        await sse_handler(writer, state)

    elif method == 'POST' and path == '/cmd':
        await cmd_handler(writer, body, state)

    else:
        await send_response(writer, 404, 'Not Found', 'text/plain')

    try:
        writer.close()
    except Exception:
        pass


async def serve_file(writer, filepath, content_type):
    try:
        with open(filepath, 'rb') as f:
            data = f.read()
        header = (
            f"HTTP/1.1 200 OK\r\n"
            f"Content-Type: {content_type}\r\n"
            f"Content-Length: {len(data)}\r\n"
            f"Connection: close\r\n"
            f"\r\n"
        )
        writer.write(header.encode('utf-8'))
        writer.write(data)
        await writer.drain()
    except FileNotFoundError:
        await send_response(writer, 500, 'File not found', 'text/plain')


async def send_response(writer, code, body, content_type='text/plain; charset=utf-8'):
    data = body.encode('utf-8')
    header = (
        f"HTTP/1.1 {code} OK\r\n"
        f"Content-Type: {content_type}\r\n"
        f"Content-Length: {len(data)}\r\n"
        f"Connection: close\r\n"
        f"Access-Control-Allow-Origin: *\r\n"
        f"\r\n"
    )
    writer.write(header.encode('utf-8'))
    writer.write(data)
    await writer.drain()


async def sse_handler(writer, state):
    """Server-Sent Events 长连接"""
    header = (
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/event-stream\r\n"
        "Cache-Control: no-cache\r\n"
        "Connection: keep-alive\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "\r\n"
    )
    writer.write(header.encode('utf-8'))
    await writer.drain()

    q = asyncio.Queue(maxsize=256)
    state.add_sse_queue(q)

    # Send initial snapshot
    await state.send_snapshot(q)

    try:
        while True:
            payload = await q.get()
            try:
                writer.write(payload.encode('utf-8'))
                await writer.drain()
            except Exception:
                break
    except asyncio.CancelledError:
        pass
    finally:
        state.remove_sse_queue(q)


async def cmd_handler(writer, body, state):
    cmd = body.strip()
    if not cmd:
        await send_response(writer, 400, 'Empty command')
        return

    print(f"[CMD] 浏览器指令: {cmd}")

    if not state.stm32_connected:
        await send_response(writer, 503, 'STM32 not connected')
        return

    # The command needs to reach the TCP bridge. We set a flag that the async
    # HTTP handler can't directly call into TcpBridge, so we store the command
    # in a queue that the main loop will process.
    await _pending_commands.put(cmd)
    await send_response(writer, 200, 'OK', 'text/plain')


# Global queue for browser → STM32 commands
_pending_commands = None


# ═══════════════════════════════════════════════════════════════
#  Main
# ═══════════════════════════════════════════════════════════════
async def main():
    global _pending_commands
    _pending_commands = asyncio.Queue()

    state = StateManager()
    bridge = TcpBridge(state)

    # Start TCP bridge
    tcp_server = await bridge.start()

    # Process pending commands
    async def process_commands():
        while True:
            cmd = await _pending_commands.get()
            await bridge.send_to_stm(cmd)

    # Start HTTP server
    async def http_serve():
        server = await asyncio.start_server(
            lambda r, w: http_handler(r, w, state),
            '0.0.0.0', HTTP_PORT)
        print(f"[HTTP] Web 服务运行在 http://localhost:{HTTP_PORT}")
        print(f"[HTTP] 本机IP: {LOCAL_IP}")
        return server

    http_server = await http_serve()
    cmd_task = asyncio.create_task(process_commands())

    print()
    print("═" * 48)
    print("  SDM18 钻具控制系统 - Web 控制面板")
    print("═" * 48)
    print(f"  本机 IP:     {LOCAL_IP}")
    print(f"  Web 页面:    http://localhost:{HTTP_PORT}")
    print(f"  TCP 桥接:    {LOCAL_IP}:{TCP_PORT}")
    print("─" * 48)
    print(f"  ESP8266 连接地址: {LOCAL_IP}:{TCP_PORT}")
    print(f"  浏览器已自动打开，如未打开请手动访问:")
    print(f"  http://localhost:{HTTP_PORT}")
    print("═" * 48)
    print()

    # Run until cancelled
    try:
        async with tcp_server:
            async with http_server:
                await asyncio.gather(
                    tcp_server.serve_forever(),
                    http_server.serve_forever(),
                )
    except asyncio.CancelledError:
        pass
    finally:
        cmd_task.cancel()


if __name__ == '__main__':
    # Open browser
    url = f'http://localhost:{HTTP_PORT}'
    print(f"正在打开浏览器: {url}")
    webbrowser.open(url)

    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        print("\n服务器已停止。")
