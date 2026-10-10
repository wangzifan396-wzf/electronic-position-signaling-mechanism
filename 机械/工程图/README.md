# Thesis engineering drawings / 论文工程图

[Mechanical directory / 返回机械目录](../) · [Repository guide / 仓库说明](../../README.md)

Added on **2026-10-10** from the author's original drawing set. The outer tube, rack, gear and inner-tube drawings correspond to the engineering views in thesis Figures 3-1 to 3-4. The simulated-core drawing is included from the same source set. The drawings and exported files are retained as supplied, without redrawing or changing dimensions.

2026-10-10新增作者原成套工程图。外管、齿条、齿轮、内管与论文终稿图3-1至图3-4中的工程视图对应，模拟岩芯为同套资料中的第五组图纸。原图及导出文件原样保留，没有重绘或改写尺寸。

## Five drawing groups / 五组工程图

PDF files contain the full original drawing sheet and title block. Use PDF to read dimensions, PNG for a quick preview, and SLDDRW to edit in SolidWorks.

PDF保留完整原图纸及题栏，建议用PDF查看尺寸、PNG快速浏览，需编辑时使用SolidWorks原工程图。

| Component / 部件 | PDF drawing / 完整图纸 | PNG preview / 预览 | SolidWorks original / 原工程图 | Thesis figure / 终稿图号 |
|---|---|---|---|---|
| Outer tube / 外管 | [PDF](PDF/90%E5%A4%96%E7%AE%A1.pdf) | [PNG](PNG/90%E5%A4%96%E7%AE%A1_01.png) | [SLDDRW](SolidWorks/90%E5%A4%96%E7%AE%A1.SLDDRW) | 图3-1 |
| Inner tube / 内管 | [PDF](PDF/65%E5%86%85%E7%AE%A1.pdf) | [PNG](PNG/65%E5%86%85%E7%AE%A1_01.png) | [SLDDRW](SolidWorks/65%E5%86%85%E7%AE%A1.SLDDRW) | 图3-4 |
| Rack / 齿条 | [PDF](PDF/%E9%BD%BF%E6%9D%A1.pdf) | [PNG](PNG/%E9%BD%BF%E6%9D%A1_01.png) | [SLDDRW](SolidWorks/%E9%BD%BF%E6%9D%A1.SLDDRW) | 图3-2 |
| Gear / 齿轮 | [PDF](PDF/%E9%BD%BF%E8%BD%AE.pdf) | [PNG](PNG/%E9%BD%BF%E8%BD%AE_01.png) | [SLDDRW](SolidWorks/%E9%BD%BF%E8%BD%AE.SLDDRW) | 图3-3 |
| Simulated core / 模拟岩芯 | [PDF](PDF/%E6%A8%A1%E6%8B%9F%E5%B2%A9%E8%8A%AF.pdf) | [PNG](PNG/%E6%A8%A1%E6%8B%9F%E5%B2%A9%E8%8A%AF_01.png) | [SLDDRW](SolidWorks/%E6%A8%A1%E6%8B%9F%E5%B2%A9%E8%8A%AF.SLDDRW) | — |

`SolidWorks/` contains **six native CAD files: five SLDDRW drawings and one SLDPRT rack model**. The same-source [rack model](SolidWorks/%E9%BD%BF%E6%9D%A1.SLDPRT) is kept next to its drawing. It differs from the original rack model in the parent mechanical directory and does not replace that file.

`SolidWorks/`共有**6个CAD原文件：5个SLDDRW工程图、1个SLDPRT齿条模型**。同源齿条模型与其工程图一同保存，与上级机械目录中的原齿条模型分别保留。

## Original thesis images / 论文原图

These four PNG files were extracted as the exact embedded image bytes from the thesis. They preserve the author's original combined engineering views and model screenshots. They are distinct from the full-sheet PNG exports above.

以下4幅PNG为论文中嵌入图片的原字节提取，保留作者原有的设计视图与模型截图组合；它们与上面的完整工程图PNG导出文件分别保存。

| Thesis figure / 论文图号 | Original image / 原图 | Corresponding drawing group / 对应工程图组 |
|---|---|---|
| 图3-1 | [论文原图 / Original image](%E8%AE%BA%E6%96%87%E5%8E%9F%E5%9B%BE/%E5%9B%BE3-1_%E5%A4%96%E7%AE%A1.png) | 90外管 |
| 图3-4 | [论文原图 / Original image](%E8%AE%BA%E6%96%87%E5%8E%9F%E5%9B%BE/%E5%9B%BE3-4_%E5%86%85%E7%AE%A1%E6%80%BB%E6%88%90.png) | 65内管 |
| 图3-2 | [论文原图 / Original image](%E8%AE%BA%E6%96%87%E5%8E%9F%E5%9B%BE/%E5%9B%BE3-2_%E9%BD%BF%E6%9D%A1.png) | 齿条 |
| 图3-3 | [论文原图 / Original image](%E8%AE%BA%E6%96%87%E5%8E%9F%E5%9B%BE/%E5%9B%BE3-3_%E9%BD%BF%E8%BD%AE.png) | 齿轮 |

## File integrity and opening CAD files / 文件校验与CAD打开说明

[DRAWING_MANIFEST.json](DRAWING_MANIFEST.json) records the relative path, size and SHA-256 of each of the **20 original files**: five PDFs, five PNG previews, five SLDDRW drawings, one SLDPRT rack model and four thesis images. The manifest and this guide are additional documentation, not part of that 20-file count.

清单记录20个原文件的路径、大小及SHA-256：5个PDF、5个PNG、5个SLDDRW、1个SLDPRT和4幅论文原图。校验清单与本说明属于另增文档，不计入20个原文件。

The native CAD files retain their original references. Their external dependencies have not been resolved or rebuilt in SolidWorks during this update. When opening a drawing, locate the referenced part or assembly if SolidWorks requests it; the PDFs can be viewed independently. Publication of these files is not a new manufacturing review or a validation of rebuilt CAD dependencies.

CAD原件保留原引用，本次未在SolidWorks中重新解析依赖或重建。打开图纸时，如软件提示缺少零件或装配体，请定位对应模型；PDF不依赖CAD模型即可查看。本次公开原图不代表新增制造校审或已验证CAD依赖完整。

The author's self-drawn material is covered by the repository's [MIT license](../../LICENSE). Third-party supplied models and vendor material retain their original terms; see [Third-party notices](../../THIRD_PARTY_NOTICES.md).

作者自绘资料采用根目录MIT许可；第三方随附模型与厂商资料继续遵循原许可，详见上述第三方说明。
