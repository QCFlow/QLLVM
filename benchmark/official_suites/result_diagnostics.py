"""Short, evidence-based explanations for failed benchmark records."""


def failure_reason(rows):
    statuses = {r['status'] for r in rows}
    error = next((r.get('error') for r in rows if r.get('error')), '')
    if 'semantic_failed' in statuses:
        return '编译输出未通过语义校验，不能计为完成。'
    if 'Shared input serialization failed:' in error:
        return '共享输入转换失败，尚未实际调用该编译器。'
    if 'PennyLane' in error and any(x in error for x in ('feedback', 'intermediate measurements', 'if_else')):
        return '当前 PennyLane 适配层不支持中间测量或经典反馈。'
    if 'no viable alternative' in error and "if(" in error:
        return '该框架的 QASM 导入器拒绝经典条件语句。'
    if 'timeout' in statuses:
        return error or '超过本用例的计时预算。'
    if statuses & {'pending', 'running', 'no_reference'}:
        return ''
    return error[:240]
