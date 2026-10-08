using PascalABCCompiler.SyntaxTree;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace Languages.SPython.Frontend.Converters
{
    internal class AssignmentCharAsStringVisitor : BaseChangeVisitor
    {
        public AssignmentCharAsStringVisitor() { }

        public override void visit(string_const literal)
        {
            // PascalABC.NET treats one-character literals as char. In Python they
            // are strings in every expression, including collection literals and
            // annotated initializers, not only on the right of an assignment.
            if (literal.Value.Length == 1)
            {
                var parent = UpperNode();
                if (parent is expression_list arguments &&
                    arguments.Parent is method_call call &&
                    call.dereferencing_value is ident name && name.name == "str")
                    return;
                Replace(literal, new method_call(new ident("str", literal.source_context),
                    new expression_list(literal, literal.source_context), literal.source_context));
            }
        }
    }
}
