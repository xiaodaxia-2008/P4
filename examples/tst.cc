// Author: Tucker Haydon

#include <cstdlib>
#include <iostream>

#include "gradient_descent.h"
#include "polynomial_sampler.h"
#include "gnuplot-iostream.h"

using namespace p4;

int main(int argc, char** argv) {

  // Set up example problem
  const std::vector<double> initial_times = {0.0, 2.0, 10.0};
  const std::vector<NodeEqualityBound> node_equality_bounds = {
    // NodeEqualityBound(dimension_idx, node_idx, derivative_idx, value)
    // Constraining position, velocity, and acceleration of first node to zero
    NodeEqualityBound(0,0,0,0),
    NodeEqualityBound(0,0,1,0),
    NodeEqualityBound(0,0,2,0),

    // Constraining position of second node
    NodeEqualityBound(0,1,0,1),

    // Constraining position, velocity, and acceleration of third node
    NodeEqualityBound(0,2,0,2.5),
    NodeEqualityBound(0,2,1,0),
    NodeEqualityBound(0,2,2,0),
  };
  const std::vector<NodeInequalityBound> node_inequality_bounds = {
  // NodeInequalityBound(dimension_idx, node_idx, derivative_idx, lower, upper)
  };
  const std::vector<SegmentInequalityBound> segment_inequality_bounds = {
    // SegmentInequalityBound(segment_idx, derivative_idx, mapping, value)
    SegmentInequalityBound(0,1,(Eigen::Matrix<double,1,1>() << 1).finished(),1),
    SegmentInequalityBound(0,1,(Eigen::Matrix<double,1,1>() << -1).finished(),1),
    SegmentInequalityBound(0,2,(Eigen::Matrix<double,1,1>() << 1).finished(),1),
    SegmentInequalityBound(0,2,(Eigen::Matrix<double,1,1>() << -1).finished(),1),

    SegmentInequalityBound(1,1,(Eigen::Matrix<double,1,1>() << 1).finished(),1),
    SegmentInequalityBound(1,1,(Eigen::Matrix<double,1,1>() << -1).finished(),1),
    SegmentInequalityBound(1,2,(Eigen::Matrix<double,1,1>() << 1).finished(),1),
    SegmentInequalityBound(1,2,(Eigen::Matrix<double,1,1>() << -1).finished(),1),
  };

  // Configure solver options
  PolynomialSolver::Options solver_options;
  solver_options.num_dimensions   = 1;
  solver_options.polynomial_order = 7;
  solver_options.derivative_order = 4;
  solver_options.continuity_order = 2;
  solver_options.osqp_settings.verbose     = 0;
  solver_options.num_nodes                 = initial_times.size();
  solver_options.node_equality_bounds      = node_equality_bounds;
  solver_options.node_inequality_bounds    = node_inequality_bounds;
  solver_options.segment_inequality_bounds = segment_inequality_bounds;

  // Perform gradient descent
  GradientDescent::Options gradient_descent_options;
  gradient_descent_options.solver_options = solver_options;
  GradientDescent gradient_descent(gradient_descent_options);
  GradientDescent::Solution gradient_descent_solution =
    gradient_descent.Run(initial_times);

  {
    std::cout << "Initial Data:" << std::endl;
    auto times = initial_times;
    auto solver = std::make_shared<PolynomialSolver>(solver_options);
    PolynomialSolver::Solution solver_solution = solver->Run(times);
    double cost = solver_solution.workspace->info->obj_val;
    std::cout << "Time: " << Eigen::Map<const Eigen::Matrix<double, Eigen::Dynamic, 1>>(
        times.data(), times.size()).transpose() << std::endl;
    std::cout << "Cost: " << cost << std::endl;
    std::cout << "" << std::endl;
  }

  {
    std::cout << "Final Data:" << std::endl;
    auto times = gradient_descent_solution.times;
    double cost = gradient_descent_solution.solver_solution.workspace->info->obj_val;
    std::cout << "Time: " << Eigen::Map<const Eigen::Matrix<double, Eigen::Dynamic, 1>>(
        times.data(), times.size()).transpose() << std::endl;
    std::cout << "Cost: " << cost << std::endl;
    std::cout << "" << std::endl;
  }

  // Extract output
  const std::vector<double> times = gradient_descent_solution.times;
  PolynomialSolver::Solution solver_solution = 
    gradient_descent_solution.solver_solution;

  // Print some output info
  // Reference: https://osqp.org/docs/interfaces/cc++#info
  std::cout << "Status:                    " << solver_solution.workspace->info->status << std::endl;
  std::cout << "Status Val (1 == success): " << solver_solution.workspace->info->status_val << std::endl;
  std::cout << "Optimal Cost:              " << solver_solution.workspace->info->obj_val << std::endl;

  // Sampling and Plotting
  { // Plot acceleration profiles
    PolynomialSampler::Options sampler_options;
    sampler_options.frequency = 100;
    sampler_options.derivative_order = 2;

    PolynomialSampler sampler(sampler_options);
    Eigen::MatrixXd samples = sampler.Run(times, solver_solution);

    std::vector<double> t_hist, x_hist;
    for(size_t time_idx = 0; time_idx < samples.cols(); ++time_idx) {
      t_hist.push_back(samples(0,time_idx));
      x_hist.push_back(samples(1,time_idx));
    }

    Gnuplot gp;
    gp << "plot '-' using 1:2 with lines title 'X-Acceleration'";
    gp << std::endl;
    gp.send1d(boost::make_tuple(t_hist, x_hist));
    gp << "set grid" << std::endl;
    gp << "replot" << std::endl;
  }

  { // Plot velocity profiles
    PolynomialSampler::Options sampler_options;
    sampler_options.frequency = 100;
    sampler_options.derivative_order = 1;

    PolynomialSampler sampler(sampler_options);
    Eigen::MatrixXd samples = sampler.Run(times, solver_solution);

    std::vector<double> t_hist, x_hist;
    for(size_t time_idx = 0; time_idx < samples.cols(); ++time_idx) {
      t_hist.push_back(samples(0,time_idx));
      x_hist.push_back(samples(1,time_idx));
    }

    Gnuplot gp;
    gp << "plot '-' using 1:2 with lines title 'X-Velocity'";
    gp << std::endl;
    gp.send1d(boost::make_tuple(t_hist, x_hist));
    gp << "set grid" << std::endl;
    gp << "replot" << std::endl;
  }

  { // Plot velocity profiles
    PolynomialSampler::Options sampler_options;
    sampler_options.frequency = 100;
    sampler_options.derivative_order = 0;

    PolynomialSampler sampler(sampler_options);
    Eigen::MatrixXd samples = sampler.Run(times, solver_solution);

    std::vector<double> t_hist, x_hist;
    for(size_t time_idx = 0; time_idx < samples.cols(); ++time_idx) {
      t_hist.push_back(samples(0,time_idx));
      x_hist.push_back(samples(1,time_idx));
    }

    Gnuplot gp;
    gp << "plot '-' using 1:2 with lines title 'X-Position'";
    gp << std::endl;
    gp.send1d(boost::make_tuple(t_hist, x_hist));
    gp << "set grid" << std::endl;
    gp << "replot" << std::endl;
  }

  return EXIT_SUCCESS;
}
