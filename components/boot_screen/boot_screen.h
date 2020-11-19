/*
 * Function:  boot_screen_task 
 * --------------------
 * 
 * This task performs the font render process and the creation of the plasma animation at the bottom 
 * of the animation. Once is finish the partial photogram, it sends to the display HAL.
 * 
 * Returns: Nothing
 * 
 */
void boot_screen_task();

/*
 * Function:  boot_screen_free 
 * --------------------
 * 
 * When the boot is finished and the boot screen task is eliminated, call this function to relesase
 * the resources reserved by the font converter engine.
 * 
 * Returns: Nothing
 * 
 */
void boot_screen_free();