#include "statement.h"

#include <iostream>
#include <sstream>

using namespace std;

namespace ast
{

    using runtime::Closure;
    using runtime::Context;
    using runtime::ObjectHolder;

    namespace
    {
        const string ADD_METHOD = "__add__"s;
        const string INIT_METHOD = "__init__"s;
    } // namespace

    ObjectHolder Assignment::Execute(Closure &closure, Context &context)
    {
        closure[var_name_] = rv_->Execute(closure, context);
        return closure.at(var_name_);
    }

    Assignment::Assignment(std::string var, std::unique_ptr<Statement> rv)
        : var_name_(std::move(var)), rv_(std::move(rv))
    {
    }

    VariableValue::VariableValue(const std::string &var_name)
    {
        dotted_ids_.push_back(var_name);
    }

    VariableValue::VariableValue(std::vector<std::string> dotted_ids)
        : dotted_ids_(std::move(dotted_ids))
    {
    }

    ObjectHolder VariableValue::Execute(Closure &closure, [[maybe_unused]] Context &context)
    {
        if (dotted_ids_.size() == 1)
        {
            if (closure.count(dotted_ids_[0]))
            {
                return closure.at(dotted_ids_[0]);
            }
            else
            {
                throw ::runtime_error("Unknown name"s);
            }
        }
        ObjectHolder obj = closure.at(dotted_ids_[0]);
        for (size_t i = 1; i + 1 < dotted_ids_.size(); ++i)
        {
            obj = obj.TryAs<runtime::ClassInstance>()->Fields().at(dotted_ids_[i]);
        }
        if (obj.TryAs<runtime::ClassInstance>()->Fields().count(dotted_ids_.back()))
        {
            return obj.TryAs<runtime::ClassInstance>()->Fields().at(dotted_ids_.back());
        }
        throw ::runtime_error("Unknown name"s);
    }

    unique_ptr<Print> Print::Variable(const std::string &name)
    {
        return make_unique<Print>(make_unique<VariableValue>(name));
    }

    Print::Print(unique_ptr<Statement> argument)
        : argument_(std::move(argument))
    {
    }

    Print::Print(vector<unique_ptr<Statement>> args)
        : args_(std::move(args))
    {
    }

    ObjectHolder Print::Execute(Closure &closure, Context &context)
    {
        if (argument_)
        {
            if (argument_->Execute(closure, context))
            {
                argument_->Execute(closure, context)->Print(context.GetOutputStream(), context);
            }
            else
            {
                context.GetOutputStream() << "None"s;
            }
        }
        if (!args_.empty())
        {
            for (size_t i = 0; i + 1 < args_.size(); ++i)
            {
                const auto &arg = args_[i]->Execute(closure, context);
                if (arg)
                {
                    arg->Print(context.GetOutputStream(), context);
                }
                else
                {
                    context.GetOutputStream() << "None"s;
                }
                context.GetOutputStream() << ' ';
            }
            const auto &arg = args_.back()->Execute(closure, context);
            if (arg)
            {
                arg->Print(context.GetOutputStream(), context);
            }
            else
            {
                context.GetOutputStream() << "None"s;
            }
        }
        context.GetOutputStream() << std::endl;
        return ObjectHolder::None();
    }

    MethodCall::MethodCall(std::unique_ptr<Statement> object, std::string method,
                           std::vector<std::unique_ptr<Statement>> args)
        : object_(std::move(object)), method_(std::move(method)), args_(std::move(args))
    {
    }

    ObjectHolder MethodCall::Execute(Closure &closure, Context &context)
    {
        std::vector<runtime::ObjectHolder> actual_args;
        for (const auto &arg : args_)
        {
            actual_args.push_back(arg->Execute(closure, context));
        }
        return object_->Execute(closure, context).TryAs<runtime::ClassInstance>()->Call(method_, actual_args, context);
    }

    ObjectHolder Stringify::Execute(Closure &closure, Context &context)
    {
        const ObjectHolder &argument = argument_->Execute(closure, context);
        if (argument.TryAs<runtime::Number>())
        {
            return ObjectHolder::Own(runtime::String(std::to_string(argument.TryAs<runtime::Number>()->GetValue())));
        }
        else if (argument.TryAs<runtime::String>())
        {
            return ObjectHolder::Own(runtime::String(argument.TryAs<runtime::String>()->GetValue()));
        }
        else if (argument.TryAs<runtime::ClassInstance>())
        {
            ostringstream str;
            argument.TryAs<runtime::ClassInstance>()->Print(str, context);
            return ObjectHolder::Own(runtime::String(str.str()));
        }
        else
        {
            return ObjectHolder::Own(runtime::String("None"s));
        }
    }

    ObjectHolder Add::Execute(Closure &closure, Context &context)
    {
        const ObjectHolder &lhs = lhs_->Execute(closure, context);
        const ObjectHolder &rhs = rhs_->Execute(closure, context);
        if (lhs.TryAs<runtime::Number>() && rhs.TryAs<runtime::Number>())
        {
            return ObjectHolder::Own(runtime::Number(lhs.TryAs<runtime::Number>()->GetValue() + rhs.TryAs<runtime::Number>()->GetValue()));
        }
        if (lhs.TryAs<runtime::String>() && rhs.TryAs<runtime::String>())
        {
            return ObjectHolder::Own(runtime::String(lhs.TryAs<runtime::String>()->GetValue() + rhs.TryAs<runtime::String>()->GetValue()));
        }
        if (lhs.TryAs<runtime::ClassInstance>())
        {
            if (lhs.TryAs<runtime::ClassInstance>()->HasMethod(ADD_METHOD, 1U))
            {
                return lhs.TryAs<runtime::ClassInstance>()->Call(ADD_METHOD, std::vector<ObjectHolder>{rhs}, context);
            }
        }
        throw ::runtime_error("Incorrect operation"s);
    }

    ObjectHolder Sub::Execute(Closure &closure, Context &context)
    {
        const ObjectHolder &lhs = lhs_->Execute(closure, context);
        const ObjectHolder &rhs = rhs_->Execute(closure, context);
        if (lhs.TryAs<runtime::Number>() && rhs_->Execute(closure, context).TryAs<runtime::Number>())
        {
            return ObjectHolder::Own(runtime::Number(lhs.TryAs<runtime::Number>()->GetValue() - rhs.TryAs<runtime::Number>()->GetValue()));
        }
        throw ::runtime_error("Incorrect operation"s);
    }

    ObjectHolder Mult::Execute(Closure &closure, Context &context)
    {
        const ObjectHolder &lhs = lhs_->Execute(closure, context);
        const ObjectHolder &rhs = rhs_->Execute(closure, context);
        if (lhs.TryAs<runtime::Number>() && rhs.TryAs<runtime::Number>())
        {
            return ObjectHolder::Own(runtime::Number(lhs.TryAs<runtime::Number>()->GetValue() * rhs.TryAs<runtime::Number>()->GetValue()));
        }
        throw ::runtime_error("Incorrect operation"s);
    }

    ObjectHolder Div::Execute(Closure &closure, Context &context)
    {
        const ObjectHolder &lhs = lhs_->Execute(closure, context);
        const ObjectHolder &rhs = rhs_->Execute(closure, context);
        if (lhs.TryAs<runtime::Number>() && rhs.TryAs<runtime::Number>() && rhs.TryAs<runtime::Number>()->GetValue() != 0)
        {
            return ObjectHolder::Own(runtime::Number(lhs.TryAs<runtime::Number>()->GetValue() / rhs.TryAs<runtime::Number>()->GetValue()));
        }
        throw ::runtime_error("Incorrect operation"s);
    }

    ObjectHolder Compound::Execute(Closure &closure, Context &context)
    {
        for (const auto &stmt : statements_)
        {
            stmt->Execute(closure, context);
        }
        return ObjectHolder::None();
    }

    ObjectHolder Return::Execute(Closure &closure, Context &context)
    {
        throw statement_->Execute(closure, context);
    }

    ClassDefinition::ClassDefinition(ObjectHolder cls)
        : class_(std::move(cls))
    {
    }

    ObjectHolder ClassDefinition::Execute(Closure &closure, [[maybe_unused]] Context &context)
    {
        closure[class_.TryAs<runtime::Class>()->GetName()] = class_;
        return class_;
    }

    FieldAssignment::FieldAssignment(VariableValue object, std::string field_name,
                                     std::unique_ptr<Statement> rv)
        : object_(std::move(object)), field_name_(std::move(field_name)), rv_(std::move(rv))
    {
    }

    ObjectHolder FieldAssignment::Execute(Closure &closure, Context &context)
    {
        const ObjectHolder &rv = rv_->Execute(closure, context);
        object_.Execute(closure, context).TryAs<runtime::ClassInstance>()->Fields()[field_name_] = rv;
        return rv;
    }

    IfElse::IfElse(std::unique_ptr<Statement> condition, std::unique_ptr<Statement> if_body,
                   std::unique_ptr<Statement> else_body)
        : condition_(std::move(condition)), if_body_(std::move(if_body)), else_body_(std::move(else_body))
    {
    }

    ObjectHolder IfElse::Execute(Closure &closure, Context &context)
    {
        if (condition_->Execute(closure, context).TryAs<runtime::Bool>()->GetValue())
        {
            return if_body_->Execute(closure, context);
        }
        else if (else_body_)
        {
            return else_body_->Execute(closure, context);
        }
        return ObjectHolder::None();
    }

    ObjectHolder Or::Execute(Closure &closure, Context &context)
    {
        const ObjectHolder &lhs = lhs_->Execute(closure, context);
        if (lhs.TryAs<runtime::Bool>()->GetValue())
        {
            return lhs;
        }
        else
        {
            return rhs_->Execute(closure, context);
        }
    }

    ObjectHolder And::Execute(Closure &closure, Context &context)
    {
        const ObjectHolder &lhs = lhs_->Execute(closure, context);
        if (!lhs.TryAs<runtime::Bool>()->GetValue())
        {
            return lhs;
        }
        else
        {
            return rhs_->Execute(closure, context);
        }
    }

    ObjectHolder Not::Execute(Closure &closure, Context &context)
    {
        return ObjectHolder::Own(runtime::Bool{!argument_->Execute(closure, context).TryAs<runtime::Bool>()->GetValue()});
    }

    Comparison::Comparison(Comparator cmp, unique_ptr<Statement> lhs, unique_ptr<Statement> rhs)
        : BinaryOperation(std::move(lhs), std::move(rhs)), cmp_(std::move(cmp))
    {
    }

    ObjectHolder Comparison::Execute(Closure &closure, Context &context)
    {
        return ObjectHolder::Own(runtime::Bool(cmp_(lhs_->Execute(closure, context), rhs_->Execute(closure, context), context)));
    }

    NewInstance::NewInstance(const runtime::Class &class_, std::vector<std::unique_ptr<Statement>> args)
        : new_class_(class_), args_(std::move(args))
    {
    }

    NewInstance::NewInstance(const runtime::Class &class_)
        : new_class_(class_)
    {
    }

    ObjectHolder NewInstance::Execute(Closure &closure, Context &context)
    {
        if (new_class_.HasMethod(INIT_METHOD, args_.size()))
        {
            std::vector<runtime::ObjectHolder> actual_args;
            for (const auto &arg : args_)
            {
                actual_args.push_back(arg->Execute(closure, context));
            }
            new_class_.Call(INIT_METHOD, actual_args, context);
        }
        return ObjectHolder::Share(new_class_);
    }

    MethodBody::MethodBody(std::unique_ptr<Statement> &&body)
        : body_(std::move(body))
    {
    }

    ObjectHolder MethodBody::Execute(Closure &closure, Context &context)
    {
        try
        {
            return body_->Execute(closure, context);
        }
        catch (ObjectHolder &obj)
        {
            return obj;
        }
        return ObjectHolder::None();
    }

} // namespace ast