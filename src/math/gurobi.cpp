#include <fstream>
#include <gurobi_c++.h>

#include "ilp_solver.h"

namespace math
{
    class MyCallback : public GRBCallback
    {
    public:
        double result_ = INFINITY;
        std::string intermediate_result_path_;
        std::vector<ILPSolver::VariableInfo> &var_info_;
        GRBVar *vars_;
        MyCallback(const std::string &intermediate_result_path, std::vector<ILPSolver::VariableInfo> &var_info, GRBVar *vars)
            : intermediate_result_path_(intermediate_result_path), var_info_(var_info), vars_(vars) {}

    protected:
        void
        callback()
        {
            if (where == GRB_CB_MIPSOL)
            {
                // 发现了一个新的整数可行解！
                VariableId numVars = var_info_.size();
                double obj = getDoubleInfo(GRB_CB_MIPSOL_OBJ);
                double *x = getSolution(vars_, numVars);
                if (obj < result_)
                    result_ = obj;
                else
                    return;

                // 立即输出到文件，防止程序崩溃导致数据丢失
                std::ofstream f(intermediate_result_path_);
                f << "Objective: " << obj << std::endl;
                f << "Number of Vars:" << numVars << std::endl;
                for (int i = 0; i < numVars; i++)
                {
                    var_info_[i].value_ = x[var_info_[i].id_];
                    if (var_info_[i].value_ > 0.5)
                    { // 针对 0/1 变量输出
                        f << var_info_[i].name_ << " = " << var_info_[i].value_
                          << std::endl;
                    }
                }
                f.close();
                delete[] x;
            }
        }
    };

    void ILPSolver::solve_gurobi(Second time_limit)
    {
        size_t var_count = var_info_.size();
        double *lb = new double[var_count];
        double *ub = new double[var_count];
        double *obj = new double[var_count];
        char *vtype = new char[var_count];
        std::string *name = new std::string[var_count];

        for (auto &var : var_info_)
        {
            switch (var.type_)
            {
            case VariableType::Binary:
                vtype[var.id_] = GRB_BINARY;
                break;
            case VariableType::Integer:
                vtype[var.id_] = GRB_INTEGER;
                break;
            case VariableType::Continuous:
                vtype[var.id_] = GRB_CONTINUOUS;
                break;
            }
            lb[var.id_] = var.lower_bound_;
            ub[var.id_] = var.upper_bound_;
            obj[var.id_] = 0.0;
            name[var.id_] = var.name_;
        }

        for (auto &[var_id, coeff] : objective_.coeff_)
            obj[var_id] = coeff;

        GRBEnv env(true);
        env.start();

        GRBModel model(env);
        GRBVar *vars = model.addVars(lb, ub, obj, vtype, name, var_count);

        for (auto &constraint : constraints_)
        {
            GRBLinExpr lhs;
            for (auto &[var_id, coeff] : constraint.coeff_)
                lhs += coeff * vars[var_id];
            switch (constraint.sense_)
            {
            case Constraint::ConstraintSense::Equal:
                model.addConstr(lhs == constraint.rhs_);
                break;
            case Constraint::ConstraintSense::LessEqual:
                model.addConstr(lhs <= constraint.rhs_);
                break;
            case Constraint::ConstraintSense::GreaterEqual:
                model.addConstr(lhs >= constraint.rhs_);
                break;
            }
        }

        switch (objective_sense_)
        {
        case ILPSolver::ObjectiveSense::Minimize:
            model.set(GRB_IntAttr_ModelSense, GRB_MINIMIZE);
            break;
        case ILPSolver::ObjectiveSense::Maximize:
            model.set(GRB_IntAttr_ModelSense, GRB_MAXIMIZE);
            break;
        default:
            break;
        }

        model.set(GRB_IntParam_Method, 2);                // Barrier方法
        model.set(GRB_IntParam_Threads, 0);               // 10个线程
        model.set(GRB_DoubleParam_TimeLimit, time_limit); // 时间限制
        model.set(GRB_DoubleParam_MIPGap, 0);             // 1%的gap
        model.set(GRB_IntParam_Presolve, 2);              // 自动预处理
        model.set(GRB_DoubleParam_NodefileStart, 0.5);    // 内存超过50%时写入磁盘
        model.set(GRB_DoubleParam_MemLimit, 128);         // 128GB内存限制

        MyCallback cb(intermediate_result_path_, var_info_, vars);
        if (!intermediate_result_path_.empty())
            model.setCallback(&cb);

        model.optimize();

        // 如果无解，找冲突条件
        if (model.get(GRB_IntAttr_SolCount) == 0)
        {
            std::cerr << "No solution found. Computing IIS..." << std::endl;
            model.computeIIS();
            model.write("model.ilp");
            std::cerr << "IIS written to model.ilp" << std::endl;
        }

        objective_value_ = model.get(GRB_DoubleAttr_ObjVal);

        for (auto &var : var_info_)
            var.value_ = vars[var.id_].get(GRB_DoubleAttr_X);

        delete[] lb;
        delete[] ub;
        delete[] obj;
        delete[] vtype;
        delete[] name;
    }
}
