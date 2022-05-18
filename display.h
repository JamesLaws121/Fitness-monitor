/*
 * display.h
 *
 *  Created on: May 18, 2022
 *      Author: Dell Latitude
 */

#ifndef DISPLAY_H_
#define DISPLAY_H_


void lineUpdate(char *str1, uint16_t num, char *str2, uint8_t charLine);

void displayUpdate(int* step_count,int* step_goal);

void processUserInput(int* step_count,int* step_goal);


#endif /* DISPLAY_H_ */
