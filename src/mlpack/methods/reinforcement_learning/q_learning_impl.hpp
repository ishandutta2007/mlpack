/**
 * @file methods/reinforcement_learning/q_learning_impl.hpp
 * @author Shangtong Zhang
 *
 * This file is the implementation of QLearning class.
 *
 * mlpack is free software; you may redistribute it and/or modify it under the
 * terms of the 3-clause BSD license.  You should have received a copy of the
 * 3-clause BSD license along with mlpack.  If not, see
 * http://www.opensource.org/licenses/BSD-3-Clause for more information.
 */
#ifndef MLPACK_METHODS_RL_Q_LEARNING_IMPL_HPP
#define MLPACK_METHODS_RL_Q_LEARNING_IMPL_HPP

#include "q_learning.hpp"

namespace mlpack {
namespace rl {

template <
  typename EnvironmentType,
  typename NetworkType,
  typename UpdaterType,
  typename PolicyType,
  typename ReplayType
>
QLearning<
  EnvironmentType,
  NetworkType,
  UpdaterType,
  PolicyType,
  ReplayType
>::QLearning(TrainingConfig& config,
             NetworkType& network,
             PolicyType& policy,
             ReplayType& replayMethod,
             UpdaterType updater,
             EnvironmentType environment):
    config(config),
    learningNetwork(network),
    policy(policy),
    replayMethod(replayMethod),
    updater(std::move(updater)),
    #if ENS_VERSION_MAJOR >= 2
    updatePolicy(NULL),
    #endif
    environment(std::move(environment)),
    totalSteps(0),
    deterministic(false)
{
  // Set up q-learning network.
  if (learningNetwork.Parameters().is_empty())
    learningNetwork.ResetParameters();

  #if ENS_VERSION_MAJOR == 1
  this->updater.Initialize(learningNetwork.Parameters().n_rows,
                           learningNetwork.Parameters().n_cols);
  #else
  this->updatePolicy = new typename UpdaterType::template
      Policy<arma::mat, arma::mat>(this->updater,
                                   learningNetwork.Parameters().n_rows,
                                   learningNetwork.Parameters().n_cols);
  #endif

  targetNetwork = learningNetwork;
}

template <
  typename EnvironmentType,
  typename NetworkType,
  typename UpdaterType,
  typename PolicyType,
  typename ReplayType
>
QLearning<
  EnvironmentType,
  NetworkType,
  UpdaterType,
  PolicyType,
  ReplayType
>::~QLearning()
{
  #if ENS_VERSION_MAJOR >= 2
  delete updatePolicy;
  #endif
}

template <
  typename EnvironmentType,
  typename NetworkType,
  typename UpdaterType,
  typename PolicyType,
  typename ReplayType
>
arma::Col<size_t> QLearning<
  EnvironmentType,
  NetworkType,
  UpdaterType,
  PolicyType,
  ReplayType
>::BestAction(const arma::mat& actionValues)
{
  // Take best possible action at a particular instance.
  arma::Col<size_t> bestActions(actionValues.n_cols);
  arma::rowvec maxActionValues = arma::max(actionValues, 0);
  for (size_t i = 0; i < actionValues.n_cols; ++i)
  {
    bestActions(i) = arma::as_scalar(
        arma::find(actionValues.col(i) == maxActionValues[i], 1));
  }
  return bestActions;
};

template <
  typename EnvironmentType,
  typename NetworkType,
  typename UpdaterType,
  typename BehaviorPolicyType,
  typename ReplayType
>
void QLearning<
  EnvironmentType,
  NetworkType,
  UpdaterType,
  BehaviorPolicyType,
  ReplayType
>::TrainAgent()
{
  // Start experience replay.

  // Sample from previous experience.
  arma::mat sampledStates;
  std::vector<ActionType> sampledActions;
  arma::colvec sampledRewards;
  arma::mat sampledNextStates;
  arma::icolvec isTerminal;

  replayMethod.Sample(sampledStates, sampledActions, sampledRewards,
      sampledNextStates, isTerminal);

  // Compute action value for next state with target network.
  arma::mat nextActionValues;
  targetNetwork.Predict(sampledNextStates, nextActionValues);

  arma::Col<size_t> bestActions;
  if (config.DoubleQLearning())
  {
    // If use double Q-Learning, use learning network to select the best action.
    arma::mat nextActionValues;
    learningNetwork.Predict(sampledNextStates, nextActionValues);
    bestActions = BestAction(nextActionValues);
  }
  else
  {
    bestActions = BestAction(nextActionValues);
  }

  // Compute the update target.
  arma::mat target;
  learningNetwork.Forward(sampledStates, target);

  double discount = std::pow(config.Discount(), replayMethod.NSteps());

  /**
   * If the agent is at a terminal state, then we don't need to add the
   * discounted reward. At terminal state, the agent wont perform any
   * action.
   */
  for (size_t i = 0; i < sampledNextStates.n_cols; ++i)
  {
    target(sampledActions[i].action, i) = sampledRewards(i) + discount *
        nextActionValues(bestActions(i), i) * (1 - isTerminal[i]);
  }

  // Learn from experience.
  arma::mat gradients;
  learningNetwork.Backward(sampledStates, target, gradients);

  replayMethod.Update(target, sampledActions, nextActionValues, gradients);

  #if ENS_VERSION_MAJOR == 1
  updater.Update(learningNetwork.Parameters(), config.StepSize(), gradients);
  #else
  updatePolicy->Update(learningNetwork.Parameters(), config.StepSize(),
      gradients);
  #endif

  if (config.NoisyQLearning() == true)
  {
    learningNetwork.ResetNoise();
    targetNetwork.ResetNoise();
  }
  // Update target network.
  if (totalSteps % config.TargetNetworkSyncInterval() == 0)
    targetNetwork = learningNetwork;

  if (totalSteps > config.ExplorationSteps())
    policy.Anneal();
}

template <
  typename EnvironmentType,
  typename NetworkType,
  typename UpdaterType,
  typename BehaviorPolicyType,
  typename ReplayType
>
void QLearning<
  EnvironmentType,
  NetworkType,
  UpdaterType,
  BehaviorPolicyType,
  ReplayType
>::TrainCategoricalAgent()
{
  arma::colvec params = {2.2145,2.2145,2.2145,2.2145,2.2145,2.2145,2.2145,2.2145,
                      -0.274,-0.274, 
                      2.2145,2.2145,2.2145,2.2145,2.2145,2.2145,2.2145,2.2145,
                      -0.274,-0.274, -0.274,-0.274};
  learningNetwork.Parameters() = arma::mat(params);
  targetNetwork.Parameters() = arma::mat(params);
  // Start experience replay.

  // Sample from previous experience.
  arma::mat sampledStates;
  std::vector<ActionType> sampledActions;
  arma::colvec sampledRewards;
  arma::mat sampledNextStates;
  arma::icolvec isTerminal;

  replayMethod.Sample(sampledStates, sampledActions, sampledRewards,
      sampledNextStates, isTerminal);

  double vMin = 0, vMax = 200.0;
  size_t atomSize = 2;
  arma::rowvec support = arma::linspace<arma::rowvec>(vMin, vMax, atomSize);

  std::cout << "support: " << support << std::endl;

  size_t batchSize = sampledNextStates.n_cols;

  // Compute action value for next state with target network.
  arma::mat nextActionValues;
  targetNetwork.Predict(sampledNextStates, nextActionValues);

  std::cout << "nextActionValues: " << nextActionValues << std::endl;

  arma::Col<size_t> nextAction;
  if (config.DoubleQLearning())
  {
    // If use double Q-Learning, use learning network to select the best action.
    arma::mat nextActionValues;
    learningNetwork.Predict(sampledNextStates, nextActionValues);
    nextAction = BestAction(nextActionValues);
  }
  else
  {
    nextAction = BestAction(nextActionValues);
  }

  arma::mat nextDists, nextDist(atomSize, batchSize);
  targetNetwork.Forward(sampledNextStates, nextDists);
  for (size_t i = 0; i < batchSize; ++i)
  {
    nextDist.col(i) = nextDists(nextAction(i) * atomSize, i,
        arma::size(atomSize, 1));
  }

  arma::mat tZ = (arma::conv_to<arma::mat>::from(config.Discount() *
      ((1 - isTerminal) * support)).each_col() + sampledRewards).t();
  tZ = arma::clamp(tZ, vMin, vMax);
  arma::mat b = (tZ - vMin) / (vMax - vMin) * (atomSize - 1);
  arma::mat l = arma::floor(b);
  arma::mat u = arma::ceil(b);
  // arma::umat offset(atomSize, batchSize);
  // offset.each_row() = arma::linspace<arma::urowvec>(0, (batchSize - 1) *
  //   atomSize, batchSize);
    
  arma::mat projDistUpper = nextDist % (u - b);
  arma::mat projDistLower = nextDist % (b - l);

  arma::mat projDist = arma::zeros<arma::mat>(arma::size(nextDist));
  for (size_t batchNo = 0; batchNo < batchSize; batchNo++)
  {
    for (size_t j = 0; j < atomSize; j++)
    {
      projDist(l(j, batchNo), batchNo) += projDistUpper(j, batchNo);
      projDist(u(j, batchNo), batchNo) += projDistLower(j, batchNo);
    }
  }
  arma::mat dists;
  learningNetwork.Forward(sampledStates, dists);

  std::cout << "dists: " << dists << std::endl;

  arma::mat lossGradients = arma::zeros<arma::mat>(arma::size(dists));
  for (size_t i = 0; i < batchSize; ++i)
  {
    lossGradients(sampledActions[i].action * atomSize, i, arma::size(atomSize, 1))
        = -(projDist.col(i) / (1e-10 + dists(sampledActions[i].action * atomSize,
        i, arma::size(atomSize, 1))));
  }

  std::cout << "lossGradients: " << lossGradients << std::endl;

  // Learn from experience.
  arma::mat gradients;
  learningNetwork.Backward(sampledStates, lossGradients, gradients);

  std::cout << "gradients: " << gradients << std::endl;

  // TODO: verify for PER
  replayMethod.Update(lossGradients, sampledActions, nextActionValues, gradients);

  #if ENS_VERSION_MAJOR == 1
  updater.Update(learningNetwork.Parameters(), config.StepSize(), gradients);
  #else
  updatePolicy->Update(learningNetwork.Parameters(), config.StepSize(),
      gradients);
  #endif

  if (config.NoisyQLearning() == true)
  {
    learningNetwork.ResetNoise();
    targetNetwork.ResetNoise();
  }
  // Update target network.
  if (totalSteps % config.TargetNetworkSyncInterval() == 0)
    targetNetwork = learningNetwork;

  if (totalSteps > config.ExplorationSteps())
    policy.Anneal();
}

template <
  typename EnvironmentType,
  typename NetworkType,
  typename UpdaterType,
  typename BehaviorPolicyType,
  typename ReplayType
>
void QLearning<
  EnvironmentType,
  NetworkType,
  UpdaterType,
  BehaviorPolicyType,
  ReplayType
>::SelectAction()
{
  // Get the action value for each action at current state.
  arma::colvec actionValue;
  learningNetwork.Predict(state.Encode(), actionValue);

  // Select an action according to the behavior policy.
  action = policy.Sample(actionValue, deterministic, config.NoisyQLearning());
}

template <
  typename EnvironmentType,
  typename NetworkType,
  typename UpdaterType,
  typename BehaviorPolicyType,
  typename ReplayType
>
double QLearning<
  EnvironmentType,
  NetworkType,
  UpdaterType,
  BehaviorPolicyType,
  ReplayType
>::Episode()
{
  // Get the initial state from environment.
  state = environment.InitialSample();

  // Track the return of this episode.
  double totalReturn = 0.0;

  // Running until get to the terminal state.
  while (!environment.IsTerminal(state))
  {
    SelectAction();

    // Interact with the environment to advance to next state.
    StateType nextState;
    double reward = environment.Sample(state, action, nextState);

    totalReturn += reward;
    totalSteps++;

    state.Data() = {1.2, 0.1, -5, 0.8};
    action.action = CartPole::Action::actions::forward;
    reward = 1.7;
    nextState.Data() = {0.2, -0.1, -0.5, 1.8};
    replayMethod.Store(state, action, reward, nextState,
        false, config.Discount());
    // Update current state.
    state = nextState;

    if (deterministic || totalSteps < config.ExplorationSteps())
      continue;
    if (config.IsCategorical())
      TrainCategoricalAgent();
    else
      TrainAgent();    
  }
  return totalReturn;
}

} // namespace rl
} // namespace mlpack

#endif
