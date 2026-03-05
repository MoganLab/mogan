/******************************************************************************
 * MODULE     : claw_ai.hpp
 * DESCRIPTION: Claw AI integration for Mogan
 * COPYRIGHT  : (C) 2026  Gatsby
 ******************************************************************************/

#ifndef CLAW_AI_H
#define CLAW_AI_H

#include <string>

// Claw AI panel functions
void show_claw_ai_panel(bool visible);
bool claw_ai_panel_visible();
std::string claw_ai_send(const std::string& message);

// Initialization
void initialize_claw_ai();

#endif // CLAW_AI_H
