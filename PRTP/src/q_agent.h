/**
 * @file q_agent.h
 * @brief Q-learning agent for adaptive reliability selection.
 * 
 * Implements reinforcement learning agent that selects per-packet reliability
 * guarantees based on network conditions (RTT) and sensor data importance.
 * Learns optimal reliability policies through temporal difference updates.
 * 
 * @author Rahman Mehraj (rahmanmehraj627@gmail.com)
 */

#ifndef Q_AGENT_H
#define Q_AGENT_H

#include <stdio.h>

#define Q_NUM_RTT_STATES 2        /**< RTT discretization: LOW, HIGH */
#define Q_NUM_IMPORTANCE_LEVELS 3 /**< Sensor importance: LOW, NORMAL, HIGH */
#define Q_NUM_ACTIONS 3           /**< Actions: UNRELIABLE, RELIABLE, DROP */

/**
 * @enum Q_AGENT_ACTION
 * @brief Packet disposition actions.
 * 
 * Actions available to the Q-learning agent for transmission control.
 */
enum Q_AGENT_ACTION {
    Q_ACTION_UNRELIABLE = 0,  /**< Send without reliability (R=0) */
    Q_ACTION_RELIABLE = 1,    /**< Send with full reliability (R=1) */
    Q_ACTION_DROP = 2         /**< Drop packet */
};

/**
 * @struct q_agent_t
 * @brief Q-learning agent state and Q-table.
 * 
 * Maintains learning parameters, exploration strategy, and Q-value table
 * for state-action value function approximation.
 */
typedef struct {
    float alpha;    /**< Learning rate (TD step size) */
    float gamma;    /**< Discount factor for future rewards */
    float epsilon;  /**< Exploration rate (epsilon-greedy) */
    int episodes_trained;  /**< Episode counter for epsilon decay */
    /** Q-value table: Q[rtt_state][importance][action] */
    float Q[Q_NUM_RTT_STATES][Q_NUM_IMPORTANCE_LEVELS][Q_NUM_ACTIONS];
} q_agent_t;

/**
 * @brief Initialize Q-learning agent.
 * @param agent Agent instance to initialize
 * @param alpha Learning rate (typically 0.1-0.3)
 * @param gamma Discount factor (typically 0.9-0.99)
 * @param epsilon Initial exploration rate (typically 0.1-0.5)
 */
void q_agent_init(q_agent_t* agent, float alpha, float gamma, float epsilon);

/**
 * @brief Select action using epsilon-greedy strategy.
 * @param agent Agent instance
 * @param rtt_state Current RTT state discretization
 * @param importance Current data importance level
 * @return Selected action (unreliable, reliable, or drop)
 */
int q_agent_choose_action(q_agent_t* agent, int rtt_state, int importance);

/**
 * @brief Update Q-table using temporal difference learning.
 * @param agent Agent instance
 * @param rtt_state Previous RTT state
 * @param importance Previous importance level
 * @param action Executed action
 * @param reward Observed reward signal
 * @param next_rtt_state Subsequent RTT state
 * @param next_importance Subsequent importance level
 */
void q_agent_update(q_agent_t* agent, 
                    int rtt_state, int importance, int action, 
                    float reward,
                    int next_rtt_state, int next_importance);

/**
 * @brief Persist Q-table to CSV file.
 * @param agent Agent instance
 * @param filepath Output file path
 */
void q_agent_save(q_agent_t* agent, const char* filepath);

/**
 * @brief Load Q-table from CSV file.
 * @param agent Agent instance
 * @param filepath Input file path
 * @return 0 on success, negative on error
 */
int q_agent_load(q_agent_t* agent, const char* filepath);

/**
 * @brief Display Q-table for analysis.
 * @param agent Agent instance
 * @param out Output file stream
 */
void q_agent_print_table(q_agent_t* agent, FILE* out);

/**
 * @brief Map sensor type and value to importance level.
 * @param sensor_type Semantic sensor type identifier
 * @param value Sensor reading or measurement value
 * @return Importance level (0=LOW, 1=NORMAL, 2=HIGH)
 */
int q_agent_semantic_to_importance(const char* sensor_type, const char* value);

/**
 * @brief Decay exploration rate after training episode.
 * @param agent Agent instance
 */
void q_agent_decay_epsilon(q_agent_t* agent);

#endif /* Q_AGENT_H */
