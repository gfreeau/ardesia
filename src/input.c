/* 
 * Ardesia -- a program for painting on the screen
 * with this program you can play, draw, learn and teach
 * This program has been written such as a freedom sonet
 * We believe in the freedom and in the freedom of education
 *
 * Copyright (C) 2009 Pilolli Pietro <pilolli.pietro@gmail.com>
 *
 * Ardesia is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * Ardesia is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */


#include <input.h>


/* Add input device. */
static void
add_input_mode_device   (AnnotateData    *data,
                         GdkDevice       *device,
                         GdkInputMode     mode)
{
  if (!data->devdatatable)
    {
      data->devdatatable = g_hash_table_new (NULL, NULL);
    }
    
  AnnotateDeviceData *devdata = (AnnotateDeviceData *) NULL;
  devdata  = g_malloc ((gsize) sizeof (AnnotateDeviceData));
  devdata->coord_list = (GSList *) NULL;
  g_hash_table_insert(data->devdatatable, device, devdata);
  
  if (!gdk_device_set_mode (device, mode))
    {
      g_warning ("Unable to set the device %s to the %d mode\n",
                  gdk_device_get_name (device),
                  mode);
    }
		
  g_printerr ("Enabled Device in screen %d. %p: \"%s\" (Type: %d)\n",
               mode,
               device,
               gdk_device_get_name (device),
               gdk_device_get_source (device));
}


/* Set-up input device list. */
static void
setup_input_device_list (AnnotateData  *data,
                         GList         *devices)
{
  remove_input_devices (data);
  GList *d = (GList *) NULL;
  for (d = devices; d; d = d->next)
    {
      GdkDevice *device = (GdkDevice *) d->data;
      add_input_device (data, device);
    }
}


/* Select the preferred input mode depending on axis. */
static GdkInputMode
select_input_device_mode     (GdkDevice     *device)
{
  if (gdk_device_get_source (device) != GDK_SOURCE_KEYBOARD && gdk_device_get_n_axes (device) >= 2)
    {
      /* Choose screen mode. */
      return GDK_MODE_SCREEN;
    }
  else
    {
      /* Choose window mode. */
      return GDK_MODE_WINDOW;
    }
}


/* Remove all the devices . */
void
remove_input_devices    (AnnotateData  *data)
{
  if (data->devdatatable)
    {
      GList* list = (GList *) NULL;
      list = g_hash_table_get_keys (data->devdatatable);
      while (list) 
        {
          GdkDevice *device = (GdkDevice *) list->data;
          remove_input_device (data, device);
          list = g_list_next (list);
        }
      g_hash_table_remove_all (data->devdatatable);
      data->devdatatable = (GHashTable *) NULL;
    }
}


/* Set-up input devices. */
void
setup_input_devices     (AnnotateData  *data)
{
  GList *devices, *d;
  GdkDeviceManager *device_manager = gdk_display_get_device_manager (gdk_display_get_default ());
  GList *masters = gdk_device_manager_list_devices (device_manager, GDK_DEVICE_TYPE_MASTER);
  GList *slavers = gdk_device_manager_list_devices (device_manager, GDK_DEVICE_TYPE_SLAVE);
  devices = g_list_concat(masters, slavers);
  //setup_input_device_list(data, masters);
  setup_input_device_list(data, devices);
}


/* Add input device. */
void
add_input_device        (AnnotateData  *data,
                         GdkDevice     *device)
{
  /* only enable devices with 2 ore more axes and exclude keyboards */
  if ((gdk_device_get_source(device) != GDK_SOURCE_KEYBOARD) &&
      ( gdk_device_get_n_axes (device) >= 2))
    {
      add_input_mode_device (data, device, select_input_device_mode (device));
    }
}


/* Add input device. */
void
remove_input_device     (AnnotateData  *data,
                         GdkDevice     *device)
{
  AnnotateDeviceData *devdata = g_hash_table_lookup(data->devdatatable, device);;
  annotate_coord_dev_list_free (devdata);
  g_hash_table_remove (data->devdatatable, device);
}


/* Grab pointer. */
void
grab_pointer       (GtkWidget           *widget,
                    GdkEventMask         eventmask)
{
  GdkGrabStatus result;
  GdkDisplay    *display = (GdkDisplay *) NULL;
  GdkDevice     *pointer = (GdkDevice *) NULL;
  GdkDeviceManager *device_manager = (GdkDeviceManager *) NULL;

  display = gdk_display_get_default ();
  ungrab_pointer     (display);
  device_manager = gdk_display_get_device_manager (display);
  pointer = gdk_device_manager_get_client_pointer (device_manager);

  gdk_error_trap_push ();
 
  result = gdk_device_grab (pointer,
                            gtk_widget_get_window (widget),
                            GDK_OWNERSHIP_WINDOW,
                            TRUE,
                            eventmask,
                            NULL,
                            GDK_CURRENT_TIME);
 
  gdk_flush ();
  if (gdk_error_trap_pop ())
    {
      g_printerr ("Grab pointer error\n");
    }

  switch (result)
    {
    case GDK_GRAB_SUCCESS:
      break;
    case GDK_GRAB_ALREADY_GRABBED:
      g_printerr ("Grab Pointer failed: AlreadyGrabbed\n");
      break;
    case GDK_GRAB_INVALID_TIME:
      g_printerr ("Grab Pointer failed: GrabInvalidTime\n");
      break;
    case GDK_GRAB_NOT_VIEWABLE:
      g_printerr ("Grab Pointer failed: GrabNotViewable\n");
      break;
    case GDK_GRAB_FROZEN:
      g_printerr ("Grab Pointer failed: GrabFrozen\n");
      break;
    default:
      g_printerr ("Grab Pointer failed: Unknown error\n");
    }

}


/* Ungrab pointer. */
void
ungrab_pointer     (GdkDisplay        *display)
{
  GdkDevice     *pointer = (GdkDevice *) NULL;
  GdkDeviceManager *device_manager = (GdkDeviceManager *) NULL;

  display = gdk_display_get_default ();
  device_manager = gdk_display_get_device_manager (display);
  pointer = gdk_device_manager_get_client_pointer (device_manager);

  gdk_error_trap_push ();
  
  gdk_device_ungrab (pointer, GDK_CURRENT_TIME);
  gdk_flush ();
  if (gdk_error_trap_pop ())
    {
      /* this probably means the device table is outdated,
       * e.g. this device doesn't exist anymore.
       */
      g_printerr ("Ungrab pointer device error\n");
    }
}


