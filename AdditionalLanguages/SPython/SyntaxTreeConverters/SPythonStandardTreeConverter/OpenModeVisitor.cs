using PascalABCCompiler.SyntaxTree;

namespace Languages.SPython.Frontend.Converters
{
    // SPython uses static types. For a literal binary mode, select the binary
    // overload before semantic type inference so read() returns bytes.
    internal class OpenModeVisitor : BaseChangeVisitor
    {
        public override void visit(method_call call)
        {
            base.visit(call);
            if (!(call.dereferencing_value is dot_node target) ||
                !(target.left is ident unit) || unit.name != "SPythonSystem" ||
                !(target.right is ident name) || name.name != "open" ||
                call.parameters == null)
                return;

            expression mode = null;
            for (int i = 0; i < call.parameters.expressions.Count; i++)
            {
                var argument = call.parameters.expressions[i];
                if (i == 1 && !(argument is name_assign_expr))
                    mode = argument;
                if (argument is name_assign_expr named && named.name.name == "mode")
                    mode = named.expr;
            }

            if (mode is string_const literal &&
                (literal.Value == "rb" || literal.Value == "br"))
                name.name = "!open_binary";
        }
    }
}
