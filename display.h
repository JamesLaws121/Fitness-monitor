/**************************************
 * display.h
 *
 * Created on: May 18, 2022
 *     Author: J.Laws, D. Beukenholdt
 *
 * Last modified: 18 May 2022
 **************************************/

#ifndef DISPLAY_H_
#define DISPLAY_H_


void lineUpdate(char *str1, uint16_t num, char *str2, uint8_t charLine);

void displayUpdate(uint16_t* step_count,uint16_t* step_goal);

void processUserInput(uint16_t* step_count,uint16_t* step_goal);

void displayInit();

#endif /* DISPLAY_H_ */
