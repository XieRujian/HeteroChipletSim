#include <algorithm>
#include <cmath>
#include <stdexcept>

#include "ilp_solver.h"

namespace math
{
    double Variable::GetValue() const
    {
        return solver_ptr_->var_info_[id_].value_;
    }

    LinearExpression &LinearExpression::operator+=(const Variable &var)
    {
        if (solver_ptr_ == nullptr)
            solver_ptr_ = var.solver_ptr_;
        if (solver_ptr_ != var.solver_ptr_)
            throw std::invalid_argument("Variable belongs to a different ILPSolver.");

        coeff_[var.GetId()] += 1.0;
        return *this;
    }

    LinearExpression &LinearExpression::operator-=(const Variable &var)
    {
        if (solver_ptr_ == nullptr)
            solver_ptr_ = var.solver_ptr_;
        if (solver_ptr_ != var.solver_ptr_)
            throw std::invalid_argument("Variable belongs to a different ILPSolver.");

        coeff_[var.GetId()] -= 1.0;
        return *this;
    }

    LinearExpression &LinearExpression::operator*=(double scalar) noexcept
    {
        for (auto &[_, coeff] : coeff_)
            coeff *= scalar;
        constant_ *= scalar;
        return *this;
    }

    LinearExpression &LinearExpression::operator/=(double scalar) noexcept
    {
        for (auto &[_, coeff] : coeff_)
            coeff /= scalar;
        constant_ /= scalar;
        return *this;
    }

    LinearExpression &LinearExpression::operator+=(const LinearExpression &other)
    {
        for (const auto &[var_id, coeff] : other.coeff_)
            coeff_[var_id] += coeff;
        constant_ += other.constant_;
        return *this;
    }

    LinearExpression &LinearExpression::operator-=(const LinearExpression &other)
    {
        for (const auto &[var_id, coeff] : other.coeff_)
            coeff_[var_id] -= coeff;
        constant_ -= other.constant_;
        return *this;
    }

    LinearExpression LinearExpression::operator+(const LinearExpression &other) const
    {
        LinearExpression result = *this;
        return result += other;
    }

    LinearExpression LinearExpression::operator-(const LinearExpression &other) const
    {
        LinearExpression result = *this;
        return result -= other;
    }

    LinearExpression LinearExpression::operator+(double scalar) const
    {
        LinearExpression result = *this;
        return result += scalar;
    }

    LinearExpression LinearExpression::operator-(double scalar) const
    {
        LinearExpression result = *this;
        return result -= scalar;
    }

    LinearExpression LinearExpression::operator*(double scalar) const
    {
        LinearExpression result = *this;
        return result *= scalar;
    }

    LinearExpression LinearExpression::operator/(double scalar) const
    {
        LinearExpression result = *this;
        return result /= scalar;
    }

    LinearExpression operator-(double scalar, const LinearExpression &expr)
    {
        LinearExpression result;
        for (auto &[id, coeff] : expr.coeff_)
            result.coeff_[id] = -coeff;
        result.constant_ = scalar - expr.constant_;
        return result;
    }

    LinearExpression Variable::operator+(double scalar) const
    {
        LinearExpression result = *this;
        return result += scalar;
    }

    LinearExpression Variable::operator-(double scalar) const
    {
        LinearExpression result = *this;
        return result -= scalar;
    }

    LinearExpression Variable::operator*(double scalar) const
    {
        LinearExpression result = *this;
        return result *= scalar;
    }

    LinearExpression Variable::operator/(double scalar) const
    {
        LinearExpression result = *this;
        return result /= scalar;
    }

    Constraint Variable::operator==(double rhs) const { return Constraint(*this, Constraint::Equal, rhs); }
    Constraint Variable::operator<=(double rhs) const { return Constraint(*this, Constraint::LessEqual, rhs); }
    Constraint Variable::operator>=(double rhs) const { return Constraint(*this, Constraint::GreaterEqual, rhs); }
    Constraint LinearExpression::operator==(const LinearExpression &rhs) const { return Constraint(*this - rhs, Constraint::Equal); }
    Constraint LinearExpression::operator<=(const LinearExpression &rhs) const { return Constraint(*this - rhs, Constraint::LessEqual); }
    Constraint LinearExpression::operator>=(const LinearExpression &rhs) const { return Constraint(*this - rhs, Constraint::GreaterEqual); }

    Constraint Variable::operator==(const Variable &rhs) const
    {
        if (solver_ptr_ != rhs.solver_ptr_)
            throw std::invalid_argument("Variables belong to different ILPSolvers.");

        Constraint constraint(*this, Constraint::Equal);
        constraint.coeff_[rhs.GetId()] = -1.0;
        return constraint;
    }

    Constraint Variable::operator<=(const Variable &rhs) const
    {
        if (solver_ptr_ != rhs.solver_ptr_)
            throw std::invalid_argument("Variables belong to different ILPSolvers.");

        Constraint constraint(*this, Constraint::LessEqual);
        constraint.coeff_[rhs.GetId()] = -1.0;
        return constraint;
    }

    Constraint Variable::operator>=(const Variable &rhs) const
    {
        if (solver_ptr_ != rhs.solver_ptr_)
            throw std::invalid_argument("Variables belong to different ILPSolvers.");

        Constraint constraint(*this, Constraint::GreaterEqual);
        constraint.coeff_[rhs.GetId()] = -1.0;
        return constraint;
    }

    void ILPSolver::CreateVariable(const std::string &name, VariableType type, double lb, double ub)
    {
        if (var_name_to_id_.count(name))
            throw std::runtime_error("Variable already exists: " + name);
        var_name_to_id_[name] = var_count_;
        var_info_.emplace_back(var_count_++, name, type, lb, ub);
    }

    Variable ILPSolver::GetVariable(const std::string &name) const
    {
        auto it = var_name_to_id_.find(name);
        if (it == var_name_to_id_.end())
            throw std::runtime_error("Variable not found: " + name);
        return Variable{*this, it->second};
    }

    void ILPSolver::Solve(Second time_limit)
    {
        switch (solver_choice_)
        {
        case Gurobi:
            solve_gurobi(time_limit);
            break;
        }
    }

    void ILPSolver::Clear()
    {
        var_count_ = 0;
        var_name_to_id_.clear();
        var_info_.clear();
        constraints_.clear();
        objective_ = LinearExpression();
        objective_sense_ = Minimize;
        objective_value_ = 0.0;
    }
}