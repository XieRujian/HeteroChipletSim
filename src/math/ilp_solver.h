#ifndef ILP_SOLVER_H
#define ILP_SOLVER_H

#include <cmath>
#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include "util/common.h"

namespace math
{
    typedef int64_t VariableId;

    class Variable;
    class LinearExpression;
    class Constraint;
    class ILPSolver;

    enum class VariableType
    {
        Binary,
        Integer,
        Continuous
    };

    class Variable
    {
    private:
        friend class ILPSolver;
        friend class LinearExpression;
        friend class Constraint;
        const ILPSolver *solver_ptr_;
        VariableId id_;

    public:
        Variable(const ILPSolver &solver, VariableId id) : solver_ptr_(&solver), id_(id) {}

        VariableId GetId() const noexcept { return id_; }
        double GetValue() const;

    public:
        LinearExpression operator+(double scalar) const;
        LinearExpression operator-(double scalar) const;
        LinearExpression operator*(double scalar) const;
        LinearExpression operator/(double scalar) const;

        Constraint operator==(double rhs) const;
        Constraint operator<=(double rhs) const;
        Constraint operator>=(double rhs) const;

        Constraint operator==(const Variable &rhs) const;
        Constraint operator<=(const Variable &rhs) const;
        Constraint operator>=(const Variable &rhs) const;
    };

    class LinearExpression
    {
        const ILPSolver *solver_ptr_;
        std::map<VariableId, double> coeff_;
        double constant_;

        friend class ILPSolver;
        friend class Variable;
        friend class Constraint;

    public:
        LinearExpression(double constant = 0.0) : solver_ptr_(nullptr), constant_(constant) {}
        LinearExpression(const Variable &var) : solver_ptr_(var.solver_ptr_), coeff_({{var.id_, 1.0}}), constant_(0.0) {}

        LinearExpression &operator+=(const Variable &var);
        LinearExpression &operator-=(const Variable &var);
        LinearExpression &operator+=(double scalar) noexcept { return constant_ += scalar, *this; }
        LinearExpression &operator-=(double scalar) noexcept { return constant_ -= scalar, *this; }

        LinearExpression &operator+=(const LinearExpression &other);
        LinearExpression &operator-=(const LinearExpression &other);
        LinearExpression &operator*=(double scalar) noexcept;
        LinearExpression &operator/=(double scalar) noexcept;

        LinearExpression operator+(const LinearExpression &other) const;
        LinearExpression operator-(const LinearExpression &other) const;
        LinearExpression operator+(double scalar) const;
        LinearExpression operator-(double scalar) const;
        LinearExpression operator*(double scalar) const;
        LinearExpression operator/(double scalar) const;
        friend LinearExpression operator+(double scalar, const LinearExpression &expr) { return expr + scalar; }
        friend LinearExpression operator-(double scalar, const LinearExpression &expr);
        friend LinearExpression operator*(double scalar, const LinearExpression &expr) { return expr * scalar; }

        Constraint operator==(const LinearExpression &rhs) const;
        Constraint operator<=(const LinearExpression &rhs) const;
        Constraint operator>=(const LinearExpression &rhs) const;
    };

    class Constraint
    {
    public:
        enum class ConstraintSense
        {
            Equal,
            LessEqual,
            GreaterEqual
        };

    private:
        const ILPSolver *solver_ptr_;
        std::map<VariableId, double> coeff_;
        ConstraintSense sense_;
        double rhs_;

        friend class LinearExpression;
        friend class Variable;
        friend class ILPSolver;

    public:
        Constraint() : solver_ptr_(nullptr), sense_(ConstraintSense::Equal), rhs_(0.0) {}
        Constraint(const Variable &var, ConstraintSense sense, double rhs = 0) : solver_ptr_(var.solver_ptr_), coeff_({{var.id_, 1.0}}), sense_(sense), rhs_(rhs) {}
        Constraint(const LinearExpression &expr, ConstraintSense sense, double rhs = 0) : solver_ptr_(expr.solver_ptr_), coeff_(expr.coeff_), sense_(sense), rhs_(rhs - expr.constant_) {}
    };

    class ILPSolver
    {
    public:
        enum class SolverChoice
        {
            Gurobi,
            // CPLEX,
            // CBC
        };

        enum class ObjectiveSense
        {
            Minimize,
            Maximize
        };

        struct VariableInfo
        {
            VariableId id_;
            VariableType type_;
            std::string name_;
            double lower_bound_;
            double upper_bound_;
            double value_;

            VariableInfo(VariableId id, const std::string &name, VariableType type, double lb, double ub) : id_(id), type_(type), name_(name), lower_bound_(lb), upper_bound_(ub) {}
        };

    private:
        VariableId var_count_;
        std::map<std::string, VariableId> var_name_to_id_;
        std::vector<VariableInfo> var_info_;
        std::vector<Constraint> constraints_;
        LinearExpression objective_;
        ObjectiveSense objective_sense_;

        double objective_value_;
        std::string intermediate_result_path_;

        friend class Variable;

    private:
        SolverChoice solver_choice_;
        void solve_gurobi(Second time_limit);

    public:
        ILPSolver() : var_count_(0), solver_choice_(SolverChoice::Gurobi) {}
        ~ILPSolver() {}

        size_t GetConstraintCount() const noexcept { return constraints_.size(); }
        double GetObjectiveValue() const noexcept { return objective_value_; }

        void CreateVariable(const std::string &name, VariableType type = VariableType::Continuous, double lb = -INFINITY, double ub = INFINITY);
        void AddConstraint(const Constraint &constraint) { constraints_.push_back(constraint); }
        void SetObjective(const LinearExpression &objective, ObjectiveSense sense = ObjectiveSense::Minimize) { objective_ = objective, objective_sense_ = sense; }

        void SetIntermediateResultPath(const std::string &path) { intermediate_result_path_ = path; }
        void Solve(Second time_limit = 21600);
        void Clear();

        Variable GetVariable(const std::string &name) const;
    };
}

#endif // ILP_SOLVER_H