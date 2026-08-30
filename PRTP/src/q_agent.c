#include "q_agent.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* Reward hyperparameters */
static const float IMPORTANCE_VALUE[] = {1.0, 5.0, 20.0};
static const float LARGE_PENALTY = -100.0;
static const float LOSS_MULTIPLIER = -10.0;

void q_agent_init(q_agent_t* agent, float alpha, float gamma, float epsilon) {
    agent->alpha = alpha;
    agent->gamma = gamma;
    agent->epsilon = epsilon;
    agent->episodes_trained = 0;
    memset(agent->Q, 0, sizeof(agent->Q));
}

int q_agent_choose_action(q_agent_t* agent, int rtt_state, int importance) {
    if (rtt_state < 0 || rtt_state >= Q_NUM_RTT_STATES ||
        importance < 0 || importance >= Q_NUM_IMPORTANCE_LEVELS) {
        return 0; /* default: send unreliable */
    }
    
    /* Epsilon-greedy */
    if ((float)rand() / RAND_MAX < agent->epsilon) {
        return rand() % Q_NUM_ACTIONS;
    }
    
    /* Choose greedy action */
    float max_q = agent->Q[rtt_state][importance][0];
    int best_action = 0;
    for (int a = 1; a < Q_NUM_ACTIONS; a++) {
        if (agent->Q[rtt_state][importance][a] > max_q) {
            max_q = agent->Q[rtt_state][importance][a];
            best_action = a;
        }
    }
    return best_action;
}

void q_agent_update(q_agent_t* agent,
                    int rtt_state, int importance, int action,
                    float reward,
                    int next_rtt_state, int next_importance) {
    if (rtt_state < 0 || rtt_state >= Q_NUM_RTT_STATES ||
        importance < 0 || importance >= Q_NUM_IMPORTANCE_LEVELS ||
        action < 0 || action >= Q_NUM_ACTIONS) {
        return;
    }
    if (next_rtt_state < 0 || next_rtt_state >= Q_NUM_RTT_STATES ||
        next_importance < 0 || next_importance >= Q_NUM_IMPORTANCE_LEVELS) {
        return;
    }
    
    /* Find best next action */
    float best_next_q = agent->Q[next_rtt_state][next_importance][0];
    for (int a = 1; a < Q_NUM_ACTIONS; a++) {
        if (agent->Q[next_rtt_state][next_importance][a] > best_next_q) {
            best_next_q = agent->Q[next_rtt_state][next_importance][a];
        }
    }
    
    /* TD update */
    float td_error = reward + agent->gamma * best_next_q - 
                     agent->Q[rtt_state][importance][action];
    agent->Q[rtt_state][importance][action] += agent->alpha * td_error;
}

void q_agent_save(q_agent_t* agent, const char* filepath) {
    FILE* f = fopen(filepath, "w");
    if (!f) return;
    
    fprintf(f, "rtt,importance,action,q_value\n");
    for (int r = 0; r < Q_NUM_RTT_STATES; r++) {
        for (int i = 0; i < Q_NUM_IMPORTANCE_LEVELS; i++) {
            for (int a = 0; a < Q_NUM_ACTIONS; a++) {
                fprintf(f, "%d,%d,%d,%.6f\n", r, i, a, agent->Q[r][i][a]);
            }
        }
    }
    fclose(f);
}

int q_agent_load(q_agent_t* agent, const char* filepath) {
    FILE* f = fopen(filepath, "r");
    if (!f) return -1;
    
    char line[256];
    fgets(line, sizeof(line), f); /* skip header */
    
    while (fgets(line, sizeof(line), f)) {
        int r, i, a;
        float q;
        if (sscanf(line, "%d,%d,%d,%f", &r, &i, &a, &q) == 4) {
            if (r >= 0 && r < Q_NUM_RTT_STATES &&
                i >= 0 && i < Q_NUM_IMPORTANCE_LEVELS &&
                a >= 0 && a < Q_NUM_ACTIONS) {
                agent->Q[r][i][a] = q;
            }
        }
    }
    fclose(f);
    return 0;
}

void q_agent_print_table(q_agent_t* agent, FILE* out) {
    fprintf(out, "\n=== Q-Table ===\n");
    fprintf(out, "RTT_STATE | IMPORTANCE | ACTION | Q_VALUE\n");
    fprintf(out, "----------|------------|--------|--------\n");
    for (int r = 0; r < Q_NUM_RTT_STATES; r++) {
        for (int i = 0; i < Q_NUM_IMPORTANCE_LEVELS; i++) {
            for (int a = 0; a < Q_NUM_ACTIONS; a++) {
                fprintf(out, "    %d     |     %d      |   %d    | %.4f\n",
                        r, i, a, agent->Q[r][i][a]);
            }
        }
    }
}

int q_agent_semantic_to_importance(const char* sensor_type, const char* value) {
    if (!sensor_type || !value) return 1; /* default NORMAL */
    
    /* Camera: NO_MOTION -> LOW, MOTION_DETECTED -> HIGH */
    if (strcmp(sensor_type, "camera") == 0) {
        if (strcmp(value, "NO_MOTION") == 0) return 0;
        if (strcmp(value, "MOTION_DETECTED") == 0) return 2;
    }
    
    /* Temperature: always NORMAL */
    if (strcmp(sensor_type, "temp") == 0) return 1;
    
    /* Status change: HIGH */
    if (strcmp(sensor_type, "device") == 0 && strcmp(value, "STATUS_CHANGE") == 0) {
        return 2;
    }
    
    return 1; /* default NORMAL */
}

void q_agent_decay_epsilon(q_agent_t* agent) {
    agent->episodes_trained++;
    if (agent->episodes_trained > 0 && agent->episodes_trained % 100 == 0) {
        agent->epsilon *= 0.995;
        if (agent->epsilon < 0.01) agent->epsilon = 0.01;
    }
}
