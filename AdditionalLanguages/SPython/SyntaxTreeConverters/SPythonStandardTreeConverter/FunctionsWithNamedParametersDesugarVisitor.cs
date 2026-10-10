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
            if (_method_call.dereferencing_value is dot_node plotCall &&
                plotCall.right is ident plotFunction &&
                _method_call.parameters is expression_list plotArguments &&
                plotArguments.expressions.Any(e => e is name_assign_expr))
            {
                // Matplotlib's facade has ordinary Pascal overloads. Rewrite
                // Python keyword arguments to positional slots before the
                // generic **kwargs lowering creates a nonexistent !plot class.
                string[] names = null;
                expression[] defaults = null;
                var nil = new nil_const();
                var nan = new double_const(double.NaN);
                switch (plotFunction.name)
                {
                    case "plot":
                        names = new[] { "x", "y", "fmt", "color", "label", "linewidth", "linestyle", "marker", "alpha" };
                        defaults = new expression[] { null, null, nil, nil, nil, nan, nil, nil, nan };
                        break;
                    case "scatter":
                        names = new[] { "x", "y", "s", "c", "marker", "cmap", "label", "alpha" };
                        defaults = new expression[] { null, null, nil, nil, nil, nil, nil, nan };
                        break;
                    case "bar":
                        names = new[] { "x", "height", "width", "color", "label" };
                        defaults = new expression[] { null, null, nan, nil, nil };
                        break;
                    case "hist":
                        names = new[] { "x", "bins", "color", "label", "density" };
                        defaults = new expression[] { null, new int32_const(-1), nil, nil, new bool_const(false) };
                        break;
                    case "imshow":
                        names = new[] { "x", "cmap", "interpolation" };
                        defaults = new expression[] { null, nil, nil };
                        break;
                    case "figure":
                        names = new[] { "num", "figsize", "dpi" };
                        defaults = new expression[] { new int32_const(-1), nil, nan };
                        break;
                    case "subplots":
                        names = new[] { "nrows", "ncols", "figsize", "sharex", "sharey" };
                        defaults = new expression[] { new int32_const(1), new int32_const(1), nil,
                            new bool_const(false), new bool_const(false) };
                        break;
                    case "savefig":
                        names = new[] { "fname", "dpi", "bbox_inches", "transparent" };
                        defaults = new expression[] { null, nan, nil, new bool_const(false) };
                        break;
                }
                if (names != null &&
                    (plotCall.left is ident plotModule && plotModule.name == "pyplot1" ||
                     plotCall.left is ident || plotCall.left is dot_node))
                {
                    expression[] slots = new expression[names.Length];
                    int positional = 0, last = -1;
                    bool namedStarted = false;
                    foreach (expression argument in plotArguments.expressions)
                    {
                        int index;
                        expression value;
                        if (argument is name_assign_expr named)
                        {
                            namedStarted = true;
                            index = Array.IndexOf(names, named.name.name);
                            if (index < 0)
                                throw new SPythonSyntaxVisitorError("UNKNOWN_NAME_{0}",
                                    argument.source_context, named.name.name);
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
                    for (int i = 0; i < names.Length; i++)
                        if (defaults[i] == null && slots[i] == null)
                            throw new SPythonSyntaxVisitorError("UNKNOWN_NAME_{0}",
                                _method_call.source_context, names[i]);
                    var normalized = new expression_list();
                    for (int i = 0; i <= last; i++)
                    {
                        var argument = slots[i] ?? defaults[i].TypedClone() as expression;
                        if (argument.source_context == null)
                            argument.source_context = _method_call.source_context;
                        normalized.Add(argument);
                    }
                    _method_call.parameters = normalized;
                    base.visit(_method_call);
                    return;
                }
            }
            if (_method_call.dereferencing_value is dot_node reCall &&
                reCall.right is ident reFunction &&
                _method_call.parameters is expression_list reArguments &&
                reArguments.expressions.Any(e => e is name_assign_expr))
            {
                bool moduleCall = reCall.left is ident reModule && reModule.name == "re1";
                string[] names = null;
                expression[] defaults = null;
                switch (reFunction.name)
                {
                    case "compile" when moduleCall:
                        names = new[] { "pattern", "flags" };
                        defaults = new expression[] { null, new int32_const(0) };
                        break;
                    case "search":
                    case "match":
                    case "fullmatch":
                    case "findall":
                    case "finditer":
                        names = moduleCall
                            ? new[] { "pattern", "string", "flags" }
                            : new[] { "string", "pos", "endpos" };
                        defaults = moduleCall
                            ? new expression[] { null, null, new int32_const(0) }
                            : new expression[] { null, new int32_const(0), new int32_const(-1) };
                        break;
                    case "split":
                        names = moduleCall
                            ? new[] { "pattern", "string", "maxsplit", "flags" }
                            : new[] { "string", "maxsplit" };
                        defaults = moduleCall
                            ? new expression[] { null, null, new int32_const(0), new int32_const(0) }
                            : new expression[] { null, new int32_const(0) };
                        break;
                    case "sub":
                    case "subn":
                        names = moduleCall
                            ? new[] { "pattern", "repl", "string", "count", "flags" }
                            : new[] { "repl", "string", "count" };
                        defaults = moduleCall
                            ? new expression[] { null, null, null, new int32_const(0), new int32_const(0) }
                            : new expression[] { null, null, new int32_const(0) };
                        break;
                }
                if (names != null)
                {
                    expression[] slots = new expression[names.Length];
                    int positional = 0;
                    int last = -1;
                    bool namedStarted = false;
                    foreach (expression argument in reArguments.expressions)
                    {
                        int index;
                        expression value;
                        if (argument is name_assign_expr named)
                        {
                            namedStarted = true;
                            index = Array.IndexOf(names, named.name.name);
                            if (index < 0)
                                throw new SPythonSyntaxVisitorError("UNKNOWN_NAME_{0}",
                                    argument.source_context, named.name.name);
                            value = named.expr;
                        }
                        else
                        {
                            if (namedStarted)
                                throw new SPythonSyntaxVisitorError("ARG_AFTER_KWARGS",
                                    argument.source_context);
                            index = positional++;
                            value = argument;
                        }
                        if (index >= slots.Length || slots[index] != null)
                            throw new SPythonSyntaxVisitorError("ARG_AFTER_KWARGS",
                                argument.source_context);
                        slots[index] = value;
                        last = Math.Max(last, index);
                    }
                    for (int i = 0; i < slots.Length && defaults[i] == null; i++)
                        if (slots[i] == null)
                            throw new SPythonSyntaxVisitorError("UNKNOWN_NAME_{0}",
                                _method_call.source_context, names[i]);
                    expression_list normalized = new expression_list();
                    for (int i = 0; i <= last; i++)
                        normalized.Add(slots[i] ?? defaults[i]);
                    _method_call.parameters = normalized;
                    base.visit(_method_call);
                    return;
                }
            }
            if (_method_call.dereferencing_value is dot_node sortedCall &&
                sortedCall.left is ident sortedModule && sortedModule.name == "SPythonSystem" &&
                sortedCall.right is ident sortedFunction && sortedFunction.name == "sorted" &&
                _method_call.parameters is expression_list sortedArguments &&
                sortedArguments.expressions.Any(e => e is name_assign_expr))
            {
                expression iterable = null;
                expression key = null;
                expression reverse = null;
                bool namedStarted = false;
                foreach (expression argument in sortedArguments.expressions)
                {
                    if (argument is name_assign_expr named)
                    {
                        namedStarted = true;
                        if (named.name.name == "iterable" && iterable == null) iterable = named.expr;
                        else if (named.name.name == "key" && key == null) key = named.expr;
                        else if (named.name.name == "reverse" && reverse == null) reverse = named.expr;
                        else throw new SPythonSyntaxVisitorError("UNKNOWN_NAME_{0}", argument.source_context, named.name.name);
                    }
                    else
                    {
                        if (namedStarted || iterable != null)
                            throw new SPythonSyntaxVisitorError("ARG_AFTER_KWARGS", argument.source_context);
                        iterable = argument;
                    }
                }
                if (iterable == null)
                    throw new SPythonSyntaxVisitorError("UNKNOWN_NAME_{0}", _method_call.source_context, "iterable");
                expression_list normalized = new expression_list();
                normalized.Add(iterable);
                if (key != null) normalized.Add(key);
                if (reverse != null) normalized.Add(reverse);
                _method_call.parameters = normalized;
                base.visit(_method_call);
                return;
            }
            if (_method_call.dereferencing_value is dot_node subnetCall &&
                subnetCall.right is ident subnetFunction &&
                (subnetFunction.name == "subnets" || subnetFunction.name == "supernet") &&
                _method_call.parameters is expression_list subnetArguments &&
                subnetArguments.expressions.Any(e => e is name_assign_expr))
            {
                expression diff = null;
                expression newPrefix = null;
                bool namedStarted = false;
                foreach (expression argument in subnetArguments.expressions)
                {
                    if (argument is name_assign_expr named)
                    {
                        namedStarted = true;
                        if (named.name.name == "prefixlen_diff" && diff == null) diff = named.expr;
                        else if (named.name.name == "new_prefix" && newPrefix == null) newPrefix = named.expr;
                        else throw new SPythonSyntaxVisitorError("UNKNOWN_NAME_{0}", argument.source_context, named.name.name);
                    }
                    else
                    {
                        if (namedStarted) throw new SPythonSyntaxVisitorError("ARG_AFTER_KWARGS", argument.source_context);
                        if (diff == null) diff = argument;
                        else if (newPrefix == null) newPrefix = argument;
                        else throw new SPythonSyntaxVisitorError("ARG_AFTER_KWARGS", argument.source_context);
                    }
                }
                expression_list normalized = new expression_list();
                if (diff != null || newPrefix != null) normalized.Add(diff ?? new int32_const(1));
                if (newPrefix != null) normalized.Add(newPrefix);
                _method_call.parameters = normalized;
                base.visit(_method_call);
                return;
            }
            if (_method_call.dereferencing_value is dot_node ipaddressCall &&
                ipaddressCall.left is ident ipaddressModule && ipaddressModule.name == "ipaddress1" &&
                ipaddressCall.right is ident ipaddressFunction &&
                (ipaddressFunction.name == "ip_network" || ipaddressFunction.name == "IPv4Network" ||
                 ipaddressFunction.name == "IPv6Network") &&
                _method_call.parameters is expression_list networkArguments &&
                networkArguments.expressions.Any(e => e is name_assign_expr))
            {
                expression address = null;
                expression strict = null;
                bool namedStarted = false;
                foreach (expression argument in networkArguments.expressions)
                {
                    if (argument is name_assign_expr named)
                    {
                        namedStarted = true;
                        if (named.name.name == "address" && address == null) address = named.expr;
                        else if (named.name.name == "strict" && strict == null) strict = named.expr;
                        else throw new SPythonSyntaxVisitorError("UNKNOWN_NAME_{0}", argument.source_context, named.name.name);
                    }
                    else
                    {
                        if (namedStarted) throw new SPythonSyntaxVisitorError("ARG_AFTER_KWARGS", argument.source_context);
                        if (address == null) address = argument;
                        else if (strict == null) strict = argument;
                        else throw new SPythonSyntaxVisitorError("ARG_AFTER_KWARGS", argument.source_context);
                    }
                }
                if (address == null) throw new SPythonSyntaxVisitorError("UNKNOWN_NAME_{0}", _method_call.source_context, "address");
                expression_list normalized = new expression_list();
                normalized.Add(address);
                if (strict != null) normalized.Add(strict);
                _method_call.parameters = normalized;
                base.visit(_method_call);
                return;
            }
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
