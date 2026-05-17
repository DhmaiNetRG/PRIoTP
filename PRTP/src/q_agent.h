#ifndef Q_AGENT_H
#define Q_AGENT_H

#include <stdio.h>

/* Q-Learning Agent for PRTP packet importance-based reliability selection
 * 
 * States:
 *   RTT: 0 = LOW (~50ms), 1 = HIGH (~300ms)
 *   Importance: 0 = LOW, 1 = NORMAL, 2 = HIGH
 * 
 * Actions:
 *   0 = Send Unreliable (R=0)
 *   1 = Send Reliable (R=1)
 *   2 = Drop packet
*/

#define Q_NUM_RTT_STATES 2
#define Q_NUM_IMPORTANCE_LEVELS 3
#define Q_NUM_ACTIONS 3

typedef struct {
    float alpha;    /* learning rate */
    float gamma;    /* discount factor */
    float epsilon;  /* exploration rate */
    int episodes_trained;  /* counter for epsilon decay */
    /* Q-table: [rtt_state][importance][action] */
    float Q[Q_NUM_RTT_STATES][Q_NUM_IMPORTANCE_LEVELS][Q_NUM_ACTIONS];
} q_agent_t;

/* Initialize agent */
void q_agent_init(q_agent_t* agent, float alpha, float gamma, float epsilon);

/* Choose action using epsilon-greedy */
int q_agent_choose_action(q_agent_t* agent, int rtt_state, int importance);

/* Update Q-table using temporal difference */
void q_agent_update(q_agent_t* agent, 
                    int rtt_state, int importance, int action, 
                    float reward,
                    int next_rtt_state, int next_importance);

/* Save Q-table to CSV file */
void q_agent_save(q_agent_t* agent, const char* filepath);

/* Load Q-table from CSV file */
int q_agent_load(q_agent_t* agent, const char* filepath);

/* Print Q-table for debugging */
void q_agent_print_table(q_agent_t* agent, FILE* out);

/* Map sensor semantic to importance level */
int q_agent_semantic_to_importance(const char* sensor_type, const char* value);

/* Decay epsilon after episode */
void q_agent_decay_epsilon(q_agent_t* agent);

#endif /* Q_AGENT_H */
