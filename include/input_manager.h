#ifndef INPUT_MANAGER_H
#define INPUT_MANAGER_H

/**
 * Initialize the input manager.
 * @return 0 on success, -1 on failure
 */
int init_input_manager(void);

/**
 * Clean up and release resources used by the input manager.
 */
void cleanup_input_manager(void);

#endif /* INPUT_MANAGER_H */