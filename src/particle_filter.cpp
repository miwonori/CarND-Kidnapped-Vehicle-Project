/**
 * particle_filter.cpp
 *
 * Created on: Dec 12, 2016
 * Author: Tiffany Huang
 */

#include "particle_filter.h"

#include <math.h>
#include <algorithm>
#include <iostream>
#include <iterator>
#include <numeric>
#include <random>
#include <string>
#include <vector>

#include "helper_functions.h"

using std::string;
using std::vector;

void ParticleFilter::init(double x, double y, double theta, double std[]) {
  /**
   * TODO: Set the number of particles. Initialize all particles to 
   *   first position (based on estimates of x, y, theta and their uncertainties
   *   from GPS) and all weights to 1. 
   * TODO: Add random Gaussian noise to each particle.
   * NOTE: Consult particle_filter.h for more information about this method 
   *   (and others in this file).
   */
  num_particles = 100;  // TODO: Set the number of particles
  std::default_random_engine gen;

  std::normal_distribution<double> dist_x(x, std[0]);
  std::normal_distribution<double> dist_y(y, std[1]);
  std::normal_distribution<double> dist_theta(theta, std[2]);

  for (int i = 0; i<num_particles; i++){
    Particle pt;
    pt.id = i;
    pt.x = dist_x(gen);
    pt.y = dist_y(gen);
    pt.theta = dist_theta(gen);
    pt.weight = 1;
    particles.push_back(pt);
    weights.push_back(pt.weight);
  }
  is_initialized = true;
}

void ParticleFilter::prediction(double delta_t, double std_pos[], 
                                double velocity, double yaw_rate) {
  /**
   * TODO: Add measurements to each particle and add random Gaussian noise.
   * NOTE: When adding noise you may find std::normal_distribution 
   *   and std::default_random_engine useful.
   *  http://en.cppreference.com/w/cpp/numeric/random/normal_distribution
   *  http://www.cplusplus.com/reference/random/default_random_engine/
   */
  std::default_random_engine gen;

  std::normal_distribution<double> dist_x(0, std_pos[0]);
  std::normal_distribution<double> dist_y(0, std_pos[1]);
  std::normal_distribution<double> dist_theta(0, std_pos[2]);

  for (int i = 0; i<num_particles; i++){
    double old_theta = particles[i].theta;
    particles[i].theta += yaw_rate * delta_t; 
    particles[i].x += velocity/yaw_rate*(sin(particles[i].theta)-sin(old_theta));
    particles[i].y += velocity/yaw_rate*(cos(old_theta)-cos(particles[i].theta));

    // add random Gaussian noise
    particles[i].x += dist_x(gen);
    particles[i].y += dist_y(gen);
    particles[i].theta += dist_theta(gen);
  }
}

// void ParticleFilter::dataAssociation(vector<LandmarkObs> predicted, 
//                                      vector<LandmarkObs>& observations) {
Associate ParticleFilter::dataAssociation(vector<LandmarkObs> predicted, 
                                     vector<LandmarkObs>& observations) {
  /**
   * TODO: Find the predicted measurement that is closest to each 
   *   observed measurement and assign the observed measurement to this 
   *   particular landmark.
   * NOTE: this method will NOT be called by the grading code. But you will 
   *   probably find it useful to implement this method and use it as a helper 
   *   during the updateWeights phase.
   */
  Associate associate;
  int tmp_associate;
  double tmp_x;
  double tmp_y;
  
  for (int i = 0; i < observations.size(); i++){
    double dist_max = 100;
    double x1 = observations[i].x;
    double y1 = observations[i].y;
    int tmpIndx = 0;
    for (int k = 0; k <predicted.size(); k++){
      double x2 = predicted[k].x;
      double y2 = predicted[k].y;
      double L = dist(x1,y1,x2,y2);
      if (L<dist_max){
        dist_max = L;
        tmp_associate = predicted[k].id;
        tmp_x = predicted[k].x;
        tmp_y = predicted[k].y;
      }
    }
    associate.associations.push_back(tmp_associate);
    associate.sense_x.push_back(tmp_x);
    associate.sense_y.push_back(tmp_y);
  }
  return associate;

}

void ParticleFilter::updateWeights(double sensor_range, double std_landmark[], 
                                   const vector<LandmarkObs> &observations, 
                                   const Map &map_landmarks) {
  /**
   * TODO: Update the weights of each particle using a mult-variate Gaussian 
   *   distribution. You can read more about this distribution here: 
   *   https://en.wikipedia.org/wiki/Multivariate_normal_distribution
   * NOTE: The observations are given in the VEHICLE'S coordinate system. 
   *   Your particles are located according to the MAP'S coordinate system. 
   *   You will need to transform between the two systems. Keep in mind that
   *   this transformation requires both rotation AND translation (but no scaling).
   *   The following is a good resource for the theory:
   *   https://www.willamette.edu/~gorr/classes/GeneralGraphics/Transforms/transforms2d.htm
   *   and the following is a good resource for the actual equation to implement
   *   (look at equation 3.33) http://planning.cs.uiuc.edu/node99.html
   */

  for (int i = 0; i < particles.size(); i++){

    double x_0 = particles[i].x;
    double y_0 = particles[i].y;
    double sin_theta = sin(particles[i].theta);
    double cos_theta = cos(particles[i].theta);
    Associate associate;
    double prob = 1;

    vector<LandmarkObs> predict_obs;
    for (int j=0; j <map_landmarks.landmark_list.size();j++){
      double predict_x = map_landmarks.landmark_list[j].x_f;
      double predict_y = map_landmarks.landmark_list[j].y_f;
      double L = dist(x_0,y_0,predict_x,predict_y);
      if (L <= sensor_range){
        LandmarkObs tmpObs;
        tmpObs.id = map_landmarks.landmark_list[j].id_i;
        tmpObs.x = predict_x;
        tmpObs.y = predict_y;
        predict_obs.push_back(tmpObs);
      }
    }
    // transform to Map coordinate
    vector<LandmarkObs> meas_obs;
    for (int k = 0; k < observations.size(); k++){
      double x_m = cos_theta*observations[k].x - sin_theta*observations[k].y + x_0;
      double y_m = sin_theta*observations[k].x + cos_theta*observations[k].y + y_0;
      LandmarkObs tmpObs;
      tmpObs.id = observations[k].id;
      tmpObs.x = x_m;
      tmpObs.y = y_m;
      meas_obs.push_back(tmpObs);
      // particles[i].sense_x.push_back(x_m);
      // particles[i].sense_y.push_back(y_m);
    }
    
    associate = dataAssociation(predict_obs, meas_obs);
    SetAssociations(particles[i], associate.associations, 
                    associate.sense_x, associate.sense_y );
    std::cout << particles[i].associations.size() << std::endl;

    for (int m = 0; m < meas_obs.size();m++){
      double mu_x = particles[i].sense_x[m];
      double mu_y = particles[i].sense_y[m];
      double tmp1 = 1/(2*M_PI*std_landmark[0]*std_landmark[1]);
      double tmp2 = pow((meas_obs[m].x-mu_x),2)/(2*std_landmark[0]*std_landmark[0]);
      double tmp3 = pow((meas_obs[m].y-mu_y),2)/(2*std_landmark[1]*std_landmark[1]);
      double tmp4 = exp(-(tmp2 + tmp3));
      prob *= tmp1*tmp4;
    }
    particles[i].weight = prob;
    weights[i] = particles[i].weight;
  }
  
  std::cout << "weightUpdate End line" << std::endl;
  
}

void ParticleFilter::resample() {
  /**
   * TODO: Resample particles with replacement with probability proportional 
   *   to their weight. 
   * NOTE: You may find std::discrete_distribution helpful here.
   *   http://en.cppreference.com/w/cpp/numeric/random/discrete_distribution
   */
  std::random_device rd;
  std::default_random_engine gen(rd());
  std::uniform_real_distribution<float> die(0,1);
  vector<Particle> tmp_particles ;
  
  double beta = 0;
  int index = int(die(gen)*num_particles); 
  double max_w = *max_element(weights.begin(), weights.end());

  for (int i = 0; i <num_particles; i++){
    beta += die(gen)*2*max_w;
    while (weights[index] < beta){
      beta = beta - weights[index];
      index = (index + 1) % num_particles;
    }
    tmp_particles.push_back(particles[index]);
  }
  particles = tmp_particles;
  std::cout << "resampled" << std::endl;

}

void ParticleFilter::SetAssociations(Particle& particle, 
                                     const vector<int>& associations, 
                                     const vector<double>& sense_x, 
                                     const vector<double>& sense_y) {
  // particle: the particle to which assign each listed association, 
  //   and association's (x,y) world coordinates mapping
  // associations: The landmark id that goes along with each listed association
  // sense_x: the associations x mapping already converted to world coordinates
  // sense_y: the associations y mapping already converted to world coordinates
  particle.associations= associations;
  particle.sense_x = sense_x;
  particle.sense_y = sense_y;
}

string ParticleFilter::getAssociations(Particle best) {
  vector<int> v = best.associations;
  std::stringstream ss;
  copy(v.begin(), v.end(), std::ostream_iterator<int>(ss, " "));
  string s = ss.str();
  s = s.substr(0, s.length()-1);  // get rid of the trailing space
  return s;
}

string ParticleFilter::getSenseCoord(Particle best, string coord) {
  vector<double> v;

  if (coord == "X") {
    v = best.sense_x;
  } else {
    v = best.sense_y;
  }

  std::stringstream ss;
  copy(v.begin(), v.end(), std::ostream_iterator<float>(ss, " "));
  string s = ss.str();
  s = s.substr(0, s.length()-1);  // get rid of the trailing space
  return s;
}