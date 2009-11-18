/* 
 * Annotate -- a program for painting on the screen 
 * Copyright (C) 2009 Pilolli Pietro <pilolli@fbk.eu>
 *
 * Some parts of this file are been copied from gromit
 * Copyright (C) 2000 Simon Budig <Simon.Budig@unix-ag.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */


/*
 * Start the dialog that ask to the user where save the image
 * containing the screenshot
 */
void start_color_selector_dialog(GtkToolButton   *toolbutton, gchar* workspace_dir, gchar* color);
