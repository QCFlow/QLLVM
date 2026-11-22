# QLLVM 编译性能测试报告

直接打开 [index.html](index.html)。网页内含最终汇总和逐用例结果，无需服务端或第三方资源；CSV下载在浏览器中生成。页面保留总门数、双比特门、深度、加速比数值，仅去掉数值下方备注。

当前快照：QLLVM `cfaeaae`，2026-11-23完成1058项、每项3次实测，1032项取得完整性能指标。二进制SHA-256：`3c70b9f689f1d460f4ef49933f3df80ca62ad04c218825393fe2476b3a5f89a2`。其他五框架复用已有记录；按相同输入SHA配对。完整编译耗时包含CLI启动，SDK导入与指标采集不计时。

使用Python 3.10+标准库从测试批次重新生成：

```bash
python benchmark/official_suites/report/publish.py \
  --campaign /path/to/campaign --output /path/to/index.html
```

批次需包含`results.json`、`catalog.json`、`status.json`；复用其他框架时另需`peer-reference.json`。这些是测试运行的本地产物，不随网页源码提交。发布器在临时目录计算统计，仅输出自包含HTML，不依赖PDF/Word、绘图库或历史调试目录。

统计使用三次实测中位数；门数与深度要求相同输入及共同门集，耗时要求相同输入。缩减率为`100% × (1 − GM(QLLVM/对手))`，加速比为`GM(对手耗时/QLLVM耗时)`。零门样本单独计数，不进入对数平均。失败或超时不填入性能数值。
