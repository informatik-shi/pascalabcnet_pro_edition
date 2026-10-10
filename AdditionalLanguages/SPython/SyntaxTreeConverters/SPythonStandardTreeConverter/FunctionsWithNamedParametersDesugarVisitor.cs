using PascalABCCompiler.SyntaxTree;
using System;
using System.Linq;

namespace Languages.SPython.Frontend.Converters
{
    internal class FunctionsWithNamedParametersDesugarVisitor : BaseChangeVisitor
    {
        public FunctionsWithNamedParametersDesugarVisitor() { }

        public override void visit(method_call _method_call)
        {
            if (_method_call.dereferencing_value is dot_node itertoolsCall &&
                itertoolsCall.left is ident itertoolsModule && itertoolsModule.name == "itertools1" &&
                itertoolsCall.right is ident itertoolsFunction && itertoolsFunction.name == "product" &&
                _method_call.parameters is expression_list productArguments &&
                productArguments.expressions.Any(e => e is name_assign_expr))
            {
                expression_list normalized = new expression_list();
                expression repeat = null;
                bool namedStarted = false;
                foreach (expression argument in productArguments.expressions)
                {
                    if (argument is name_assign_expr named)
                    {
                        namedStarted = true;
                        if (named.name.name != "repeat" || repeat != null)
                            throw new SPythonSyntaxVisitorError("UNKNOWN_NAME_{0}", argument.source_context, named.name.name);
                        repeat = named.expr;
                    }
                    else
                    {
                        if (namedStarted)
                            throw new SPythonSyntaxVisitorError("ARG_AFTER_KWARGS", argument.source_context);
                        normalized.Add(argument);
                    }
                }
                if (repeat != null)
                {
                    normalized.expressions.Insert(0, repeat);
                    itertoolsFunction.name = "product_repeat";
                }
                _method_call.parameters = normalized;
                base.visit(_method_call);
                return;
            }
            if (_method_call.dereferencing_value is dot_node repeatCall &&
                repeatCall.left is ident repeatModule && repeatModule.name == "itertools1" &&
                repeatCall.right is ident repeatFunction && repeatFunction.name == "repeat" &&
                _method_call.parameters is expression_list repeatArguments &&
                repeatArguments.expressions.Any(e => e is name_assign_expr))
            {
                expression_list normalized = new expression_list();
                expression times = null;
                bool namedStarted = false;
                foreach (expression argument in repeatArguments.expressions)
                {
                    if (argument is name_assign_expr named)
                    {
                        namedStarted = true;
                        if (named.name.name != "times" || times != null)
                            throw new SPythonSyntaxVisitorError("UNKNOWN_NAME_{0}", argument.source_context, named.name.name);
                        times = named.expr;
                    }
                    else
                    {
                        if (namedStarted || normalized.expressions.Count >= 1)
                            throw new SPythonSyntaxVisitorError("ARG_AFTER_KWARGS", argument.source_context);
                        normalized.Add(argument);
                    }
                }
                if (normalized.expressions.Count != 1 || times == null)
                    throw new SPythonSyntaxVisitorError("ARG_AFTER_KWARGS", _method_call.source_context);
                normalized.Add(times);
                _method_call.parameters = normalized;
                base.visit(_method_call);
                return;
            }
            if (_method_call.dereferencing_value is dot_node builtin &&
                builtin.left is ident module && module.name == "SPythonSystem" &&
                builtin.right is ident function &&
                (function.name == "open" || function.name == "!open_text" ||
                 function.name == "!open_binary") &&
                _method_call.parameters is expression_list openArguments &&
                openArguments.expressions.Any(e => e is name_assign_expr))
            {
                string[] names = { "file", "mode", "buffering", "encoding", "errors", "newline" };
                expression[] slots = new expression[6];
                int positional = 0;
                int last = -1;
                bool namedStarted = false;
                foreach (expression argument in openArguments.expressions)
                {
                    int index;
                    expression value;
                    if (argument is name_assign_expr named)
                    {
                        namedStarted = true;
                        index = Array.IndexOf(names, named.name.name);
                        if (index < 0)
                            throw new SPythonSyntaxVisitorError("UNKNOWN_NAME_{0}", argument.source_context, named.name.name);
                        value = named.expr;
                    }
                    else
                    {
                        if (namedStarted)
                            throw new SPythonSyntaxVisitorError("ARG_AFTER_KWARGS", argument.source_context);
                        index = positional++;
                        value = argument;
                    }
                    if (index >= slots.Length || slots[index] != null)
                        throw new SPythonSyntaxVisitorError("ARG_AFTER_KWARGS", argument.source_context);
                    slots[index] = value;
                    last = Math.Max(last, index);
                }
                if (slots[0] == null)
                    throw new SPythonSyntaxVisitorError("UNKNOWN_NAME_{0}", _method_call.source_context, "file");
                expression[] defaults = { null, new string_const("r"), new int32_const(-1),
                    new nil_const(), new nil_const(), new nil_const() };
                expression_list normalized = new expression_list();
                for (int i = 0; i <= last; i++)
                    normalized.Add(slots[i] ?? defaults[i]);
                _method_call.parameters = normalized;
                base.visit(_method_call);
                return;
            }
            if (_method_call.parameters is expression_list exprl) {
                expression_list args = new expression_list();
                expression_list kwargs_names_array = new expression_list();
                expression_list kwargs = new expression_list();

                foreach (var expr in exprl.expressions)
                {
                    if (expr is name_assign_expr nae)
                    {
                        kwargs_names_array.Add(new string_const(nae.name.name));
                        kwargs_names_array.source_context
                            = new SourceContext(kwargs_names_array.source_context, expr.source_context);
                        kwargs.Add(nae.expr);
                        kwargs.source_context 
                            = new SourceContext(kwargs.source_context, expr.source_context);
                    }
                    else if (kwargs.expressions.Count() == 0)
                    {
                        args.Add(expr);
                        args.source_context = new SourceContext(args.source_context, expr.source_context);
                    }
                    else throw new SPythonSyntaxVisitorError("ARG_AFTER_KWARGS", expr.source_context);
                }

                if (kwargs.expressions.Count() != 0)
                {
                    array_const_new acn = new array_const_new(kwargs_names_array, '|');
                    kwargs.expressions.Insert(0, acn);

                    ident method_name = new ident();
                    if (_method_call.dereferencing_value is ident id)
                    {
                        method_name = id;
                    }
                    else if (_method_call.dereferencing_value is dot_node dnn)
                    {
                        method_name = dnn.right as ident;
                    }

                    ident class_name = new ident("!" + method_name);
                    new_expr ne = new new_expr(new named_type_reference(class_name), kwargs, false, null);

                    dot_node dn = new dot_node(ne, new ident(method_name.name), _method_call.source_context);
                    method_call new_method_call = new method_call(dn, args, _method_call.source_context);
                    Replace(_method_call, new_method_call);
                    return;
                }
            }
            base.visit(_method_call);
        }
    }
}
