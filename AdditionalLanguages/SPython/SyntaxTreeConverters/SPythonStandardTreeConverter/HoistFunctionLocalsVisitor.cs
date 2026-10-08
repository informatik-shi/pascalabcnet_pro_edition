using PascalABCCompiler.SyntaxTree;
using System.Collections.Generic;
using System.Linq;

namespace Languages.SPython.Frontend.Converters
{
    // Python locals belong to a function even when first assigned in a with or
    // try block. PascalABC.NET locals declared there have block scope, so move
    // declarations used later to the function and leave assignments in place.
    internal class HoistFunctionLocalsVisitor : BaseChangeVisitor
    {
        public override void visit(procedure_definition function)
        {
            if (!(function.proc_body is block body))
                return;

            var locals = new List<var_statement>();
            Collect(body.program_code, locals);
            foreach (var local in locals)
            {
                var name = local.var_def.vars.idents[0].name;
                var scope = NearestStatementList(local);
                if (scope == null || !UsedOutside(body.program_code, scope, name))
                    continue;

                var type = local.var_def.vars_type ?? InferType(local, locals);
                if (type == null)
                    continue;

                if (body.defs == null)
                    body.defs = new declarations();
                var identifier = local.var_def.vars.idents[0];
                var declaration = new var_def_statement(new ident_list(new ident(name, identifier.source_context)),
                    type, null, definition_attribute.None, false, local.source_context);
                body.defs.Add(new variable_definitions(declaration, local.source_context), local.source_context);

                var replacement = local.var_def.inital_value == null
                    ? (statement)new empty_statement()
                    : new assign(new ident(name, identifier.source_context), local.var_def.inital_value,
                        Operators.Assignment, local.source_context);
                var index = scope.subnodes.IndexOf(local);
                if (index < 0)
                    continue;
                scope.subnodes[index] = replacement;
                replacement.Parent = scope;
            }
        }

        private static type_definition InferType(var_statement local, List<var_statement> locals)
        {
            if (!(local.var_def.inital_value is method_call read) ||
                !(read.dereferencing_value is dot_node method) ||
                !(method.left is ident receiver) ||
                !(method.right is ident member) || member.name != "read")
                return null;

            var source = locals.FirstOrDefault(item =>
                item.var_def.vars.idents[0].name == receiver.name);
            if (!(source?.var_def.inital_value is method_call open) ||
                !(open.dereferencing_value is dot_node target) ||
                !(target.right is ident functionName) || functionName.name != "!open_binary")
                return null;

            return new named_type_reference(new ident("bytes", local.source_context), local.source_context);
        }

        private static void Collect(syntax_tree_node node, List<var_statement> locals)
        {
            if (node == null || node is procedure_definition || node is function_lambda_definition)
                return;
            if (node is var_statement local)
                locals.Add(local);
            for (var i = 0; i < node.subnodes_count; i++)
                Collect(node[i], locals);
        }

        private static statement_list NearestStatementList(syntax_tree_node node)
        {
            for (var parent = node.Parent; parent != null; parent = parent.Parent)
                if (parent is statement_list scope)
                    return scope;
            return null;
        }

        private static bool UsedOutside(syntax_tree_node node, statement_list scope, string name)
        {
            if (node == null || node is procedure_definition || node is function_lambda_definition)
                return false;
            if (node is ident id && id.name == name &&
                !(id.Parent is dot_node dot && dot.right == id) && !IsWithin(id, scope))
                return true;
            for (var i = 0; i < node.subnodes_count; i++)
                if (UsedOutside(node[i], scope, name))
                    return true;
            return false;
        }

        private static bool IsWithin(syntax_tree_node node, syntax_tree_node ancestor)
        {
            for (var current = node; current != null; current = current.Parent)
                if (current == ancestor)
                    return true;
            return false;
        }
    }
}
