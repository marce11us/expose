#include "e.h"
#include "e_mod_main.h"

/* local subsystem functions */
typedef struct _E_Winlist_Win E_Winlist_Win;


struct _E_Winlist_Win
{
   E_Client     *client;
   Evas_Object	*thumb;
   unsigned char was_iconified : 1;
#ifdef _ALL_DESKTOPS_
   E_Desk		*old_desk;
   E_Zone		*old_zone;
#endif
};


static Eina_Bool _e_winlist_client_add(E_Client *ec, E_Zone *zone, E_Desk *desk);
static void      _e_winlist_activate(void);
static Eina_Bool _e_winlist_cb_key_down(void *data, int type, void *event);
static Eina_Bool _expose_winlist_cb_mouse_down(void *data, int type, void *event);
static Eina_Bool _expose_winlist_cb_mouse_move(void *data, int type, void *event);


/* local subsystem globals */
static Evas_Object* bg=NULL;
static E_Zone *_winlist_zone = NULL;
static Eina_List *_wins = NULL;
static Eina_List *_win_selected = NULL;
static E_Desk *_last_desk = NULL;
static E_Client *_last_client = NULL;
static int _hold_count = 0;
static int _hold_mod = 0;
static E_Winlist_Activate_Type _activate_type = 0;
static Eina_List *_handlers = NULL;
static Ecore_Window _input_window = 0;
int expose_rows=0;
int expose_cols=0;
int cur_x=0;
int cur_y=0;


static Eina_Bool
_wmclass_picked(const Eina_List *lst, const char *wmclass)
{
   const Eina_List *l;
   const char *s;

   if (!wmclass) return EINA_FALSE;

   EINA_LIST_FOREACH(lst, l, s)
     if (s == wmclass)
       return EINA_TRUE;

   return EINA_FALSE;
}

/* externally accessible functions */
int
expose_init(void)
{
   return 1;
}

int
expose_shutdown(void)
{
   expose_winlist_hide();
   return 1;
}

int
expose_winlist_expose(E_Zone *zone, E_Winlist_Filter filter)
{
   int w, h;
   Eina_List *l, *ll;
   E_Desk *desk;
   E_Client *ec;
   E_Winlist_Win *ww;
   Eina_List *wmclasses = NULL;

   E_OBJECT_CHECK_RETURN(zone, 0);
   E_OBJECT_TYPE_CHECK_RETURN(zone, E_ZONE_TYPE, 0);

#ifndef HAVE_WAYLAND_ONLY
   if (e_comp->comp_type == E_PIXMAP_TYPE_X)
     {
        _input_window = ecore_x_window_input_new(e_comp->root, 0, 0, 1, 1);
        ecore_x_window_show(_input_window);
        if (!e_grabinput_get(_input_window, 0, _input_window))
          {
             ecore_x_window_free(_input_window);
             _input_window = 0;
             return 0;
          }
     }
#endif
   if (e_comp->comp_type != E_PIXMAP_TYPE_X)
     {
        if (!e_comp_grab_input(1, 1))
          return 0;
        _input_window = e_comp->ee_win;
     }

   w = (double)zone->w * e_config->winlist_pos_size_w;
   if (w > e_config->winlist_pos_max_w) w = e_config->winlist_pos_max_w;
   else if (w < e_config->winlist_pos_min_w)
     w = e_config->winlist_pos_min_w;
   if (w > zone->w) w = zone->w;

   h = (double)zone->h * e_config->winlist_pos_size_h;
   if (h > e_config->winlist_pos_max_h) h = e_config->winlist_pos_max_h;
   else if (h < e_config->winlist_pos_min_h)
     h = e_config->winlist_pos_min_h;
   if (h > zone->h) h = zone->h;

   _winlist_zone = zone;
   e_client_move_cancel();
   e_client_resize_cancel();
   e_client_focus_track_freeze();

#ifndef HAVE_WAYLAND_ONLY
   evas_event_feed_mouse_in(e_comp->evas, 0, NULL);
   evas_event_feed_mouse_move(e_comp->evas, -1000000, -1000000, 0, NULL);
#endif

   evas_event_freeze(e_comp->evas);

   _last_client = e_client_focused_get();

   desk = e_desk_current_get(_winlist_zone);
   EINA_LIST_FOREACH(e_client_focus_stack_get(), l, ec)
     {
        Eina_Bool pick;

        // skip if we already have it in winlist
        EINA_LIST_FOREACH(_wins, ll, ww)
          {
             if (e_client_stack_bottom_get(ww->client) ==
                 e_client_stack_bottom_get(ec)) break;
          }
        if (ll) continue;
        switch (filter)
          {
           case E_WINLIST_FILTER_CLASS_WINDOWS:
             if (!_last_client)
               pick = EINA_FALSE;
             else
               pick = _last_client->icccm.class == ec->icccm.class;
             break;
           case E_WINLIST_FILTER_CLASSES:
             pick = (!_wmclass_picked(wmclasses, ec->icccm.class));
             if (pick)
               wmclasses = eina_list_append(wmclasses, ec->icccm.class);
             break;

           default:
             pick = EINA_TRUE;
          }
        if (pick) _e_winlist_client_add(ec, _winlist_zone, desk);
     }
   eina_list_free(wmclasses);

   if (!_wins)
     {
        expose_winlist_hide();
        evas_event_thaw(e_comp->evas);
        return 1;
     }

   if (e_config->winlist_list_show_other_desk_windows ||
       e_config->winlist_list_show_other_screen_windows)
     _last_desk = e_desk_current_get(_winlist_zone);
   
int tw, th;

   bg=evas_object_rectangle_add(e_comp->evas);
   tw=zone->w;
   th=zone->h;
   evas_object_resize(bg, tw, th);
   evas_object_layer_set(bg, E_LAYER_CLIENT_POPUP);
   evas_object_pass_events_set(bg, 1);
   evas_object_color_set(bg, 0, 0, 0, 200);
   evas_object_show(bg);

    float prop=(float)tw/(float)th;
	int cnt=0;

	EINA_LIST_FOREACH(_wins, l, ww)
   {
	   if( ( ww->client->desk == e_desk_current_get(_winlist_zone) || e_config->winlist_list_show_other_desk_windows ) &&
		   ( !ww->client->iconic || e_config->winlist_list_show_iconified ) )
		   cnt++;
   }


	expose_rows=ceil(sqrt((float)cnt*prop));
	expose_cols=ceil((float)cnt/(float)expose_rows);

	int xpad=20;
	int width=( (float)(tw-(expose_rows+1)*xpad)/(float)expose_rows < (float)(th-(expose_cols+1)*xpad)/(float)expose_cols ?
			(float)(tw-(expose_rows+1)*xpad)/(float)expose_rows : (float)(th-(expose_cols+1)*xpad)/(float)expose_cols );

		xpad=(float)(tw-expose_rows*width)/(float)(expose_rows+1);
	int ypad=(float)(th-expose_cols*width)/(float)(expose_cols+1);

	int r=0;
	int c=0;

	EINA_LIST_FOREACH(_wins, l, ww)
     {
		int w, h;

	   if( !(( ww->client->desk == e_desk_current_get(_winlist_zone) || e_config->winlist_list_show_other_desk_windows ) &&
		   ( !ww->client->iconic || e_config->winlist_list_show_iconified )) )
		   continue;

	   if( ww->client->iconic )
	   {
			ww->was_iconified=1;
          	e_client_uniconify(ww->client);
	   }

	  

		evas_object_geometry_get(ww->client->frame, NULL, NULL, &tw, &th);

		if ( tw >= th )
		{
			w=width;
			h=width*(float)th/(float)tw;
		}
		else
		{
			w=width*(float)tw/(float)th;
			h=width;
		}

   		evas_object_resize(ww->thumb, w, h);
   		evas_object_move(ww->thumb, xpad*(1+r)+r*width+(width-w)/2, ypad*(1+c)+c*width+(width-h)/2);
   		evas_object_layer_set(ww->thumb, E_LAYER_CLIENT_POPUP);

		if( ++r == expose_rows )
		{
			r=0;
			c++;
		}

		evas_object_show(ww->thumb);
      }


	//end custom code


   evas_event_thaw(e_comp->evas);

   E_LIST_HANDLER_APPEND(_handlers, ECORE_EVENT_KEY_DOWN, _e_winlist_cb_key_down, NULL);
   //E_LIST_HANDLER_APPEND(_handlers, ECORE_EVENT_KEY_UP, _e_winlist_cb_key_up, NULL);
   E_LIST_HANDLER_APPEND(_handlers, ECORE_EVENT_MOUSE_BUTTON_DOWN, _expose_winlist_cb_mouse_down, NULL);
   E_LIST_HANDLER_APPEND(_handlers, ECORE_EVENT_MOUSE_MOVE, _expose_winlist_cb_mouse_move, NULL);

   return 1;
}



void
expose_winlist_hide(void)
{
   E_Client *ec = NULL;
   E_Winlist_Win *ww;

   if(bg)
   {
		evas_object_hide(bg);
		evas_object_del(bg);
		bg=NULL;
   }

   if (_win_selected)
     {
        ww = _win_selected->data;
        ec = ww->client;
     }
   EINA_LIST_FREE(_wins, ww)
     {
        if ((!ec) || (ww->client != ec))
          e_object_unref(E_OBJECT(ww->client));

	   if( ww->was_iconified )
	   {
			ww->was_iconified=0;
			if( ww != eina_list_data_get(_win_selected) )
          		e_client_iconify(ww->client);
	   }

#ifdef _ALL_DESKTOPS_

		if( ww->old_desk )
		{
			e_client_desk_set(ww->client, ww->old_desk);
            e_client_zone_set(ww->client, ww->old_zone);
			ww->old_desk=NULL;
			ww->old_zone=NULL;
		}
#endif

		evas_object_hide(ww->thumb);
		evas_object_del(ww->thumb);
        free(ww);
     }
   _win_selected = NULL;

   e_client_focus_track_thaw();
   _winlist_zone = NULL;
   _hold_count = 0;
   _hold_mod = 0;
   _activate_type = 0;

   E_FREE_LIST(_handlers, ecore_event_handler_del);

#ifndef HAVE_WAYLAND_ONLY
   if (e_comp->comp_type == E_PIXMAP_TYPE_X)
     {
        if (_input_window)
          {
             e_grabinput_release(_input_window, _input_window);
             ecore_x_window_free(_input_window);
          }
     }
#endif
   if (e_comp->comp_type == E_PIXMAP_TYPE_WL)
     e_comp_ungrab_input(1, 1);
   _input_window = 0;
   if (ec)
     {
        Eina_Bool set = !ec->lock_focus_out;

        if (ec->shaded)
          {
             if (!ec->lock_user_shade)
               e_client_unshade(ec, ec->shade_dir);
          }
        if (e_config->winlist_list_move_after_select)
          {
             e_client_zone_set(ec, e_zone_current_get());
             e_client_desk_set(ec, e_desk_current_get(ec->zone));
          }
        else if (ec->desk)
          {
             if (!ec->sticky) e_desk_show(ec->desk);
          }
        if (!ec->lock_user_stacking)
          {
             evas_object_raise(ec->frame);
             e_client_raise_latest_set(ec);
          }
        if (ec->iconic)
          e_client_uniconify(ec);
        if (ec->shaded)
          e_client_unshade(ec, ec->shade_dir);
        if (set)
          {
             evas_object_focus_set(ec->frame, 1);
             e_client_focus_latest_set(ec);
          }
        e_object_unref(E_OBJECT(ec));
     }
}


void
e_winlist_modifiers_set(int mod, E_Winlist_Activate_Type type)
{
   _hold_mod = mod;
   _hold_count = 0;
   _activate_type = type;
   if (type == E_WINLIST_ACTIVATE_TYPE_MOUSE) _hold_count++;
   if (_hold_mod & ECORE_EVENT_MODIFIER_SHIFT) _hold_count++;
   if (_hold_mod & ECORE_EVENT_MODIFIER_CTRL) _hold_count++;
   if (_hold_mod & ECORE_EVENT_MODIFIER_ALT) _hold_count++;
   if (_hold_mod & ECORE_EVENT_MODIFIER_WIN) _hold_count++;
}

static Eina_Bool
_e_winlist_client_add(E_Client *ec, E_Zone *zone, E_Desk *desk)
{
   E_Winlist_Win *ww;

   if ((!ec->icccm.accepts_focus) &&
       (!ec->icccm.take_focus)) return EINA_FALSE;
   if (ec->netwm.state.skip_taskbar) return EINA_FALSE;
   if (ec->user_skip_winlist) return EINA_FALSE;

                 if (ec->desk != desk)
               {
                  if ((ec->zone) && (ec->zone != zone))
                    {
                       if (!e_config->winlist_list_show_other_screen_windows)
                         return EINA_FALSE;
                       if (ec->zone && ec->desk && (ec->desk != e_desk_current_get(ec->zone)))
                         {
                            if (!e_config->winlist_list_show_other_desk_windows)
                              return EINA_FALSE;
                         }
                    }
                  else if (!e_config->winlist_list_show_other_desk_windows)
                    return EINA_FALSE;
               }
          
     

   ww = E_NEW(E_Winlist_Win, 1);
   if (!ww) return EINA_FALSE;

#ifdef _ALL_DESKTOPS_
   ww->old_desk=NULL;
   ww->old_zone=NULL;
#endif

#if _ALL_DESKTOPS_
 	if( ww->client->desk != e_desk_current_get(_winlist_zone) )
		{
			fprintf(logfile, "trying to move window to desk\n");
			fflush(logfile);
			ww->old_desk=ww->client->desk;
			ww->old_zone=ww->client->zone;
			e_client_desk_set(ww->client, e_desk_current_get(_winlist_zone));
            e_client_zone_set(ww->client, _winlist_zone);
			evas_object_focus_set(ww->client->frame, 1);
		}
#endif

   fprintf(logfile, "show iconified is %d\n", e_config->winlist_list_show_iconified);
   fflush(logfile);

   //generate proxy thumbnail for expose
   ww->thumb=evas_object_image_filled_add(e_comp->evas);

   evas_object_image_source_set(ww->thumb, ec->frame);
   evas_object_layer_set(ww->thumb, E_LAYER_CLIENT_POPUP);

   evas_object_color_set(ww->thumb, 255, 255, 255, 200);


   ww->client = ec;
   _wins = eina_list_append(_wins, ww);
   e_object_ref(E_OBJECT(ww->client));
   return EINA_TRUE;
}

static void
_expose_activate_nth(int x, int y)
{
   Eina_List *l;
   int cnt=0;

   if( x < 0 )
	   x=0;
   if( x > expose_rows )
	   x=expose_rows;

   if( y < 0 )
	   y=0;

   if( y > expose_cols )
	   y=expose_cols;

   int n=expose_rows*x+y;
   E_Winlist_Win *ww = eina_list_nth(_wins, n);

	EINA_LIST_FOREACH(_wins, l, ww)
   {
	   if( ( ww->client->desk == e_desk_current_get(_winlist_zone) || e_config->winlist_list_show_other_desk_windows ) &&
		   ( !ww->client->iconic || e_config->winlist_list_show_iconified ) )
		   cnt++;
   }


   if ( n < cnt )
   {
	   cur_x=x;
	   cur_y=y;
   }

   if (n >= cnt) n = cnt - 1;
   l = eina_list_nth_list(_wins, n);
   if (!l) return;

	_win_selected = l;
   	_e_winlist_activate();


	EINA_LIST_FOREACH(_wins, l, ww)
   {
		evas_object_color_set(ww->thumb, 255, 255, 255, 200);
   }

   ww=eina_list_nth(_wins, n);
   if (!ww) return;

	evas_object_color_set(ww->thumb, 255, 255, 255, 255);
}


static void
_e_winlist_activate(void)
{
   E_Winlist_Win *ww;
   int ok = 0;

   if (!_win_selected) return;
   ww = _win_selected->data;

   if ((!ww->client->iconic) &&
       ((ww->client->desk == e_desk_current_get(_winlist_zone)) ||
        (ww->client->sticky)))
     ok = 1;
   if (ok)
     {
        int set = 1;
        
        if ((!ww->client->lock_user_stacking) &&
            (e_config->winlist_list_raise_while_selecting))
          evas_object_raise(ww->client->frame);
        if ((!ww->client->lock_focus_out) &&
            (e_config->winlist_list_focus_while_selecting))
          {
             if (set)
               evas_object_focus_set(ww->client->frame, 1);
          }
     }
}

static Eina_Bool
_e_winlist_cb_key_down(void *data EINA_UNUSED, int type EINA_UNUSED, void *event)
{
   Ecore_Event_Key *ev = event;

   if (ev->window != _input_window) return ECORE_CALLBACK_PASS_ON;
   if (!strcmp(ev->key, "Up"))
		_expose_activate_nth(cur_x-1, cur_y);
   else if (!strcmp(ev->key, "Down"))
		_expose_activate_nth(cur_x+1, cur_y);
   else if (!strcmp(ev->key, "Left"))
		_expose_activate_nth(cur_x, cur_y-1);
   else if (!strcmp(ev->key, "Right"))
		_expose_activate_nth(cur_x, cur_y+1);
   else if (!strcmp(ev->key, "Return"))
     expose_winlist_hide();
   else if (!strcmp(ev->key, "space"))
     expose_winlist_hide();
   else if (!strcmp(ev->key, "Escape"))
     expose_winlist_hide();

   return ECORE_CALLBACK_PASS_ON;
}


static Eina_Bool
_expose_winlist_cb_mouse_down(void *data EINA_UNUSED, int type EINA_UNUSED, void *event)
{
	Ecore_Event_Mouse_Button *ev=event;

	int x, y, w, h;
	Eina_List* l;
	E_Winlist_Win *ww;

	EINA_LIST_FOREACH(_wins, l, ww)
	{
		evas_object_geometry_get(ww->thumb, &x, &y, &w, &h);
		if (E_INSIDE(ev->x, ev->y, x, y, w, h))
		{
			_win_selected = l;
            _e_winlist_activate();
			expose_winlist_hide();
			return ECORE_CALLBACK_CANCEL;
		}
	}

	expose_winlist_hide();
	return ECORE_CALLBACK_CANCEL;
}

static Eina_Bool
_expose_winlist_cb_mouse_move(void *data EINA_UNUSED, int type EINA_UNUSED, void *event)
{
   Ecore_Event_Mouse_Move *ev;
   int x, y, w, h;

   ev = event;

   Eina_List* l;
   E_Winlist_Win *ww;


   EINA_LIST_FOREACH(_wins, l, ww)
   {
		evas_object_geometry_get(ww->thumb, &x, &y, &w, &h);
   		if (E_INSIDE(ev->x, ev->y, x, y, w, h))
		{
			evas_object_color_set(ww->thumb, 255, 255, 255, 255);
			_win_selected = l;
   			_e_winlist_activate();

		}
		else
		{
			evas_object_color_set(ww->thumb, 255, 255, 255, 200);
		}
   }

   return ECORE_CALLBACK_PASS_ON;
}
