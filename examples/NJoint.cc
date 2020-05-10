// Author: Tucker Haydon

#include "gnuplot-iostream.h"

#include <cstdlib>
#include <Eigen/Dense>

#include "polynomial_solver.h"
#include "polynomial_sampler.h"

using namespace p4;

int main(int argc, char** argv) {

  // Time in seconds
  const std::vector<double> times = {0, 1.5, 3};

  // NodeEqualityBound(dimension_idx, node_idx, derivative_idx, value)
  const std::vector<NodeEqualityBound> node_equality_bounds = {
    // Constraining position and velocity of first node to zero
    NodeEqualityBound(0,0,0,0),
    NodeEqualityBound(1,0,0,0),
    NodeEqualityBound(0,0,1,0),
    NodeEqualityBound(1,0,1,0),
    NodeEqualityBound(0,0,2,0),
    NodeEqualityBound(1,0,2,0),

    // Other nodes may constrain whatever they want
    // The second node is constraining position to (1,0)
    NodeEqualityBound(0,1,0,1),
    NodeEqualityBound(1,1,0,0),

    // The third node is contraining position to (1,1)
    NodeEqualityBound(0,2,0,1),
    NodeEqualityBound(1,2,0,1),
    NodeEqualityBound(0,2,1,0),
    NodeEqualityBound(1,2,1,0),
    NodeEqualityBound(0,2,2,0),
    NodeEqualityBound(1,2,2,0),
  };

  // NodeInequalityBound(dimension_idx, node_idx, derivative_idx, lower, upper)
  const std::vector<NodeInequalityBound> node_inequality_bounds = {
    // Constraining the z value of the third node above 0.5
    // NodeInequalityBound(2,2,0,0.5,NodeInequalityBound::INFTY),
  };

  // SegmentInequalityBound(segment_idx, derivative_idx, mapping, value)
  // Segment inequality bounds constrain a derivative of a segment to 
  //   dot(a,x) < b
  const std::vector<SegmentInequalityBound> segment_inequality_bounds = {
    // Constraining the x-acceleration of the first segment below 4 m/s^2
    SegmentInequalityBound(0,1,Eigen::Vector2d(1,0),2),
    SegmentInequalityBound(0,1,Eigen::Vector2d(0,1),2),
    SegmentInequalityBound(1,1,Eigen::Vector2d(1,0),2),
    SegmentInequalityBound(1,1,Eigen::Vector2d(0,1),2),
    SegmentInequalityBound(0,2,Eigen::Vector2d(1,0),4),
    SegmentInequalityBound(0,2,Eigen::Vector2d(0,1),4),
    SegmentInequalityBound(1,2,Eigen::Vector2d(1,0),4),
    SegmentInequalityBound(1,2,Eigen::Vector2d(0,1),4),
    SegmentInequalityBound(0,3,Eigen::Vector2d(1,0),10),
    SegmentInequalityBound(0,3,Eigen::Vector2d(0,1),10),
    SegmentInequalityBound(1,3,Eigen::Vector2d(1,0),10),
    SegmentInequalityBound(1,3,Eigen::Vector2d(0,1),10),
  };

  // Configure solver options
  PolynomialSolver::Options solver_options;
  solver_options.num_dimensions   = 2;   // 2D
  solver_options.polynomial_order = 5;   // Fit an 8th-order polynomial
  solver_options.continuity_order = 3;   // Require continuity through the 4th derivative
  solver_options.derivative_order = 2;   // Minimize the 2nd derivative (acceleration)

  // Configure the OSQP settings
  // Reference: https://osqp.org/docs/interfaces/cc++#settings
  solver_options.osqp_settings.polish = true;       // Polish the solution, getting the best answer possible
  solver_options.osqp_settings.verbose = false;     // Suppress the printout

  // Solve
  PolynomialSolver solver(solver_options);
  const PolynomialSolver::Solution solution
    = solver.Run(
        times, 
        node_equality_bounds,
        node_inequality_bounds,
        segment_inequality_bounds);

  // Print some output info
  // Reference: https://osqp.org/docs/interfaces/cc++#info
  std::cout << "Status:                    " << solution.workspace->info->status << std::endl;
  std::cout << "Status Val (1 == success): " << solution.workspace->info->status_val << std::endl;
  std::cout << "Optimal Cost:              " << solution.workspace->info->obj_val << std::endl;

  // Sampling and Plotting
  { // Plot jerk profiles
    PolynomialSampler::Options sampler_options;
    sampler_options.frequency = 100;
    sampler_options.derivative_order = 3;

    PolynomialSampler sampler(sampler_options);
    Eigen::MatrixXd samples = sampler.Run(times, solution);

    std::vector<double> t_hist, x_hist, y_hist;
    for(size_t time_idx = 0; time_idx < samples.cols(); ++time_idx) {
      t_hist.push_back(samples(0,time_idx));
      x_hist.push_back(samples(1,time_idx));
      y_hist.push_back(samples(2,time_idx));
    }

    Gnuplot gp;
    gp << "plot '-' using 1:2 with lines title 'X-Jerk'";
    gp << ", '-' using 1:2 with lines title 'Y-Jerk'";
    gp << ", '-' using 1:2 with lines title 'Z-Jerk'";
    gp << std::endl;
    gp.send1d(boost::make_tuple(t_hist, x_hist));
    gp.send1d(boost::make_tuple(t_hist, y_hist));
    gp << "set grid" << std::endl;
    gp << "replot" << std::endl;
  }
  { // Plot acceleration profiles
    PolynomialSampler::Options sampler_options;
    sampler_options.frequency = 100;
    sampler_options.derivative_order = 2;

    PolynomialSampler sampler(sampler_options);
    Eigen::MatrixXd samples = sampler.Run(times, solution);

    std::vector<double> t_hist, x_hist, y_hist;
    for(size_t time_idx = 0; time_idx < samples.cols(); ++time_idx) {
      t_hist.push_back(samples(0,time_idx));
      x_hist.push_back(samples(1,time_idx));
      y_hist.push_back(samples(2,time_idx));
    }

    Gnuplot gp;
    gp << "plot '-' using 1:2 with lines title 'X-Acceleration'";
    gp << ", '-' using 1:2 with lines title 'Y-Acceleration'";
    gp << ", '-' using 1:2 with lines title 'Z-Acceleration'";
    gp << std::endl;
    gp.send1d(boost::make_tuple(t_hist, x_hist));
    gp.send1d(boost::make_tuple(t_hist, y_hist));
    gp << "set grid" << std::endl;
    gp << "replot" << std::endl;
  }

  { // Plot velocity profiles
    PolynomialSampler::Options sampler_options;
    sampler_options.frequency = 100;
    sampler_options.derivative_order = 1;

    PolynomialSampler sampler(sampler_options);
    Eigen::MatrixXd samples = sampler.Run(times, solution);

    std::vector<double> t_hist, x_hist, y_hist;
    for(size_t time_idx = 0; time_idx < samples.cols(); ++time_idx) {
      t_hist.push_back(samples(0,time_idx));
      x_hist.push_back(samples(1,time_idx));
      y_hist.push_back(samples(2,time_idx));
    }

    Gnuplot gp;
    gp << "plot '-' using 1:2 with lines title 'X-Velocity'";
    gp << ", '-' using 1:2 with lines title 'Y-Velocity'";
    gp << ", '-' using 1:2 with lines title 'Z-Velocity'";
    gp << std::endl;
    gp.send1d(boost::make_tuple(t_hist, x_hist));
    gp.send1d(boost::make_tuple(t_hist, y_hist));
    gp << "set grid" << std::endl;
    gp << "replot" << std::endl;
  }

  { // Plot 2D x-y
    PolynomialSampler::Options sampler_options;
    sampler_options.frequency = 100;
    sampler_options.derivative_order = 0;

    PolynomialSampler sampler(sampler_options);
    Eigen::MatrixXd samples = sampler.Run(times, solution);

    std::vector<double> t_hist, x_hist, y_hist;
    for(size_t time_idx = 0; time_idx < samples.cols(); ++time_idx) {
      t_hist.push_back(samples(0,time_idx));
      x_hist.push_back(samples(1,time_idx));
      y_hist.push_back(samples(2,time_idx));
    }

    Gnuplot gp;
    gp << "plot '-' using 1:2 with lines title 'X-Y'";
    gp << std::endl;
    // gp.send1d(boost::make_tuple(t_hist, x_hist));
    gp.send1d(boost::make_tuple(x_hist, y_hist));
    // gp.send1d(boost::make_tuple(t_hist, z_hist));
    gp << "set grid" << std::endl;
    gp << "replot" << std::endl;
  }

//   { // Plot 3D position
//     PolynomialSampler::Options sampler_options;
//     sampler_options.frequency = 50;
//     sampler_options.derivative_order = 0;

//     PolynomialSampler sampler(sampler_options);
//     Eigen::MatrixXd samples = sampler.Run(times, solution);

//     std::vector<double> t_hist, x_hist, y_hist, z_hist;
//     for(size_t time_idx = 0; time_idx < samples.cols(); ++time_idx) {
//       t_hist.push_back(samples(0,time_idx));
//       x_hist.push_back(samples(1,time_idx));
//       y_hist.push_back(samples(2,time_idx));
//       z_hist.push_back(samples(3,time_idx));
//     }

//     Gnuplot gp;
//     gp << "splot '-' using 1:2:3 with lines title 'Trajectory'" << std::endl;
//     gp.send1d(boost::make_tuple(x_hist, y_hist, z_hist));
//     gp << "set grid" << std::endl;
//     gp << "replot" << std::endl;

//     // Must keep position gp in scope to rotate 3D graph
//     std::cout << "Press enter to exit." << std::endl;
//     std::cin.get();
//   }

  return EXIT_SUCCESS;
}