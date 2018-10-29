/******************************************************************************
 * Copyright 2018 The Apollo Authors. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *****************************************************************************/

/*
 * @file
 */

#include "modules/planning/open_space/dual_variable_warm_start_problem.h"

#include <algorithm>
#include <iomanip>
#include <utility>

#include "IpIpoptApplication.hpp"
#include "IpSolveStatistics.hpp"

#include "modules/common/time/time.h"
#include "modules/planning/common/planning_gflags.h"

namespace apollo {
namespace planning {

using apollo::common::time::Clock;

DualVariableWarmStartProblem::DualVariableWarmStartProblem(
    const PlannerOpenSpaceConfig& planner_open_space_config) {
  planner_open_space_config_.CopyFrom(planner_open_space_config);
}

bool DualVariableWarmStartProblem::Solve(
    const std::size_t& num_of_variables, const std::size_t& num_of_constraints,
    const std::size_t& horizon, const float& ts, const Eigen::MatrixXd& x0,
    const Eigen::MatrixXd& xF, const Eigen::MatrixXd& XYbounds,
    Eigen::MatrixXd* l_warm_up, Eigen::MatrixXd* n_warm_up) {
  DualVariableWarmStartIPOPTInterface* ptop =
      new DualVariableWarmStartIPOPTInterface(
          num_of_variables, num_of_constraints, horizon, ts, x0, xF, XYbounds);

  Ipopt::SmartPtr<Ipopt::TNLP> problem = ptop;

  // Create an instance of the IpoptApplication
  Ipopt::SmartPtr<Ipopt::IpoptApplication> app = IpoptApplicationFactory();

  app->Options()->SetStringValue("hessian_approximation", "exact");
  // TODO(QiL) : Change IPOPT settings to flag or configs
  int print_level = 0;
  app->Options()->SetIntegerValue("print_level", print_level);
  int num_iterations = 0;
  app->Options()->SetIntegerValue("max_iter", num_iterations);
  int mumps_mem_percent = 6000;
  app->Options()->SetIntegerValue("mumps_mem_percent", mumps_mem_percent);
  int max_iter = 750;
  app->Options()->SetIntegerValue("max_iter", max_iter);
  double tol = 1e-5;
  app->Options()->SetNumericValue("tol", tol);

  Ipopt::ApplicationReturnStatus status = app->Initialize();
  if (status != Ipopt::Solve_Succeeded) {
    ADEBUG << "*** Wart start problem error during initialization!";
    return false;
  }

  status = app->OptimizeTNLP(problem);

  if (status == Ipopt::Solve_Succeeded ||
      status == Ipopt::Solved_To_Acceptable_Level) {
    // Retrieve some statistics about the solve
    Ipopt::Index iter_count = app->Statistics()->IterationCount();
    ADEBUG << "*** The problem solved in " << iter_count << " iterations!";

    Ipopt::Number final_obj = app->Statistics()->FinalObjective();
    ADEBUG << "*** The final value of the objective function is " << final_obj
           << '.';
  } else {
    ADEBUG << "Return status: " << int(status);
  }

  ptop->get_optimization_results(l_warm_up, n_warm_up);

  return status == Ipopt::Solve_Succeeded ||
         status == Ipopt::Solved_To_Acceptable_Level;
}

}  // namespace planning
}  // namespace apollo