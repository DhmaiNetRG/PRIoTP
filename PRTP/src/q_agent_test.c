#include "q_agent.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

/* Simplified reward function for testing */
float compute_reward(int action, int importance, int success, int rtt_est) {
    float imp_val[] = {1.0, 5.0, 20.0};
    
    if (action == 2) { /* drop */
        return (importance == 0) ? 0.0 : -100.0;
    }
    if (action == 1) { /* reliable */
        if (success) {
            return imp_val[importance] - (rtt_est / 100.0);
        } else {
            return -100.0;
        }
    }
    /* action == 0: unreliable */
    if (success) {
        return imp_val[importance];
    } else {
        return -10.0 * imp_val[importance];
    }
}

int main(int argc, char* argv[]) {
    srand(time(NULL));
    
    q_agent_t agent;
    q_agent_init(&agent, 0.2, 0.9, 0.2);
    
    printf("Training Q-agent for 50000 episodes...\n");
    
    /* RTT loss parameters */
    float rtt_loss[] = {0.01, 0.2};  /* loss prob for RTT_LOW, RTT_HIGH */
    int rtt_ms[] = {50, 300};
    
    for (int ep = 0; ep < 50000; ep++) {
        int importance = rand() % 10 < 6 ? 0 : (rand() % 2 + 1);
        int rtt_state = rand() % 10 < 7 ? 0 : 1;
        
        int action = q_agent_choose_action(&agent, rtt_state, importance);
        
        int success;
        float rtt_est = rtt_ms[rtt_state];
        
        if (action == 2) {
            success = 0;
        } else {
            success = (float)rand() / RAND_MAX > rtt_loss[rtt_state];
        }
        
        float reward = compute_reward(action, importance, success, rtt_est);
        
        int next_rtt = rand() % 10 < 7 ? 0 : 1;
        int next_imp = rand() % 3;
        
        q_agent_update(&agent, rtt_state, importance, action, reward, next_rtt, next_imp);
        q_agent_decay_epsilon(&agent);
        
        if (ep % 5000 == 0 && ep > 0) {
            printf("Episode %d, epsilon=%.4f\n", ep, agent.epsilon);
        }
    }
    
    printf("Training complete.\n");
    q_agent_print_table(&agent, stdout);
    q_agent_save(&agent, "/tmp/q_agent_trained.csv");
    printf("Saved to /tmp/q_agent_trained.csv\n");
    
    return 0;
}
