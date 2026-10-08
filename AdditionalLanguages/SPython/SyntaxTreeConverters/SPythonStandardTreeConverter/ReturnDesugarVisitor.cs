using PascalABCCompiler;
using PascalABCCompiler.SyntaxTree;
using System.Collections.Generic;

namespace Languages.SPython.Frontend.Converters
{
    internal class ReturnDesugarVisitor : BaseChangeVisitor
    {
        private readonly Stack<bool> dynamicResults = new Stack<bool>();

        public ReturnDesugarVisitor() { }

        public override void Enter(syntax_tree_node node)
        {
            if (node is procedure_definition definition)
            {
                var resultType = (definition.proc_header as function_header)?.return_type as named_type_reference;
                dynamicResults.Push(resultType != null && resultType.names.Count == 1 &&
                                    resultType.names[0].name == "PyValue");
            }
            else if (node is function_lambda_definition)
                dynamicResults.Push(false);
            base.Enter(node);
        }

        public override void Exit(syntax_tree_node node)
        {
            if (node is procedure_definition || node is function_lambda_definition)
                dynamicResults.Pop();
            base.Exit(node);
        }

        public override void visit(return_statement _return_statement)
        {
            SourceContext sc = _return_statement.source_context;

            // return;
            if (_return_statement.expr == null)
            {
                procedure_call pc = new procedure_call(new ident("exit", sc), true, sc);
                Replace(_return_statement, pc);
            }
            // return expr;
            else
            {
                expression result = _return_statement.expr;
                if (dynamicResults.Count > 0 && dynamicResults.Peek())
                    result = new new_expr(new named_type_reference(new ident("PyValue", sc), sc),
                        new expression_list(result, sc), false, null, sc);
                statement res_assign = new assign(new ident(StringConstants.result_var_name), result, Operators.Assignment, sc);
                statement exit_call = new procedure_call(new ident("exit"), true, sc);
                statement_list new_statement = new statement_list(res_assign, sc);
                new_statement.Add(exit_call, sc);
                Replace(_return_statement, new_statement);
            }
        }
    }
}
