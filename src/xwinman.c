/* $Id: xwinman.c,v 1.3 2001/12/26 22:17:07 sybalsky Exp $ (C) Copyright Venue, All Rights Reserved  */
static char *id = "$Id: xwinman.c,v 1.3 2001/12/26 22:17:07 sybalsky Exp $ Copyright (C) Venue";



/************************************************************************/
/*									*/
/*	(C) Copyright 1989, 1990, 1990, 1991, 1992, 1993, 1994, 2000 Venue.	*/
/*	    All Rights Reserved.		*/
/*	Manufactured in the United States of America.			*/
/*									*/
/*	The contents of this file are proprietary information 		*/
/*	belonging to Venue, and are provided to you under license.	*/
/*	They may not be further distributed or disclosed to third	*/
/*	parties without the specific permission of Venue.		*/
/*									*/
/************************************************************************/


#include "version.h"




#include <stdio.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#ifdef ISC
#include <sys/bsdtypes.h>
#endif /* ISC */

#include "lispemul.h"
#include "devif.h"
#include "xdefs.h"

int Mouse_Included = FALSE;

extern Cursor WaitCursor,
  DefaultCursor,
  VertScrollCursor,
  VertThumbCursor,
  ScrollUpCursor,
  ScrollDownCursor,
  HorizScrollCursor,
  HorizThumbCursor,
  ScrollLeftCursor,
  ScrollRightCursor;

extern   DspInterface currentdsp, colordsp;

extern DLword  *EmCursorX68K
	     , *EmCursorY68K;
extern DLword  *EmMouseX68K
             , *EmMouseY68K
	     , *EmKbdAd068K
	     , *EmRealUtilin68K;
extern LispPTR *CLastUserActionCell68k;
extern MISCSTATS *MiscStats;
extern int KBDEventFlg;
extern u_char *SUNLispKeyMap;
#define KEYCODE_OFFSET 7	/* Sun Keycode offset */

#define	MOUSE_LEFT	13
#define	MOUSE_MIDDLE	15
#define	MOUSE_RIGHT	14




/* bound: return b if it is between a and c otherwise it returns a or c */
int bound( a, b, c)
     int a, b, c;
{
  if (b <= a) return( a );
  else if ( b >= c ) return( c );
  else return( b );
}





Set_BitGravity( event, dsp, window, grav )
     XButtonEvent *event;
     DspInterface dsp;
     Window window;
     int grav;
{
  XSetWindowAttributes Lisp_SetWinAttributes;
  Window OldWindow;

  /* Change Background Pixmap of Gravity Window */
  XLOCK;
  switch( dsp->BitGravity ) {
  case NorthWestGravity: OldWindow = dsp->NWGrav;
    break;
  case NorthEastGravity: OldWindow = dsp->NEGrav;
    break;
  case SouthWestGravity: OldWindow = dsp->SWGrav;
    break;
  case SouthEastGravity: OldWindow = dsp->SEGrav;
    break;
  };

  dsp->BitGravity = grav;

  XSetWindowBackgroundPixmap( event->display , OldWindow
			     , dsp->GravityOffPixmap );
  XClearWindow( event->display, OldWindow );

  XSetWindowBackgroundPixmap( event->display , window 
			     , dsp->GravityOnPixmap );
  XClearWindow( event->display, window );
  XUNLOCK;
}				/* end Set_BitGravity */


lisp_Xconfigure( dsp, x, y, lspWinWidth, lspWinHeight )
     DspInterface dsp;
       int x, y, lspWinWidth, lspWinHeight;
{
  int GravSize, Col2, Row2, Col3, Row3;

  /* The Vissible width and height changes when */
  /* we configure the window. Make them */
  /* stay within bounds. */
  dsp->Vissible.width = bound(OUTER_SB_WIDTH(dsp)+2,
			     lspWinWidth,
			     dsp->Display.width + OUTER_SB_WIDTH(dsp)) - OUTER_SB_WIDTH(dsp);
  dsp->Vissible.height = bound(OUTER_SB_WIDTH(dsp)+2,
			      lspWinHeight,
			      dsp->Display.height + OUTER_SB_WIDTH(dsp)) - OUTER_SB_WIDTH(dsp);

  GravSize = (int)(dsp->ScrollBarWidth/2) - (dsp->InternalBorderWidth);
  Col2 = dsp->Vissible.width;
  Row2 = dsp->Vissible.height;
  Col3 = dsp->Vissible.width + (int)(OUTER_SB_WIDTH(dsp)/2);
  Row3 = dsp->Vissible.height + (int)(OUTER_SB_WIDTH(dsp)/2);

  XLOCK;
  XMoveResizeWindow( dsp->display_id, dsp->DisplayWindow, 0, 0, dsp->Vissible.width, dsp->Vissible.height);
  /* Scroll bars */
  XMoveResizeWindow( dsp->display_id, dsp->VerScrollBar,
		    Col2, 0-dsp->InternalBorderWidth,		/* y */
		    dsp->ScrollBarWidth, /* width */
		    dsp->Vissible.height); /* height */
  XMoveResizeWindow( dsp->display_id,
		    dsp->HorScrollBar,
		    0-dsp->InternalBorderWidth, Row2, /* y */
		    dsp->Vissible.width,	/* width */
		    dsp->ScrollBarWidth); /* height */

  /* Scroll buttons */
  XMoveResizeWindow( dsp->display_id,
		    dsp->HorScrollButton,
		    (int)((dsp->Vissible.x * dsp->Vissible.width) / dsp->Display.width), /* x */
		    0-dsp->InternalBorderWidth,		/* y */
		    (int)((dsp->Vissible.width*dsp->Vissible.width)/ dsp->Display.width) + 1, /* width */
		     dsp->ScrollBarWidth); /* height */
  XMoveResizeWindow( dsp->display_id,
		    dsp->VerScrollButton,
		    0-dsp->InternalBorderWidth, /* x */
		    (int)((dsp->Vissible.y * dsp->Vissible.height) / dsp->Display.height), /* y */
		    dsp->ScrollBarWidth,	/* width */
		    (int)((dsp->Vissible.height*dsp->Vissible.height) / dsp->Display.height) + 1);	/* height */

  /* Gravity windows */
  XMoveResizeWindow( dsp->display_id, dsp->NWGrav, Col2, Row2, GravSize, GravSize);
  XMoveResizeWindow( dsp->display_id, dsp->NEGrav, Col3, Row2, GravSize, GravSize);
  XMoveResizeWindow( dsp->display_id, dsp->SEGrav, Col3, Row3, GravSize, GravSize);
  XMoveResizeWindow( dsp->display_id, dsp->SWGrav, Col2, Row3, GravSize, GravSize);
  Scroll( dsp, dsp->Vissible.x, dsp->Vissible.y );
  XFlush(dsp->display_id);
  XUNLOCK;
}				/* end lisp_Xconfigure */




/************************************************************************/
/*									*/
/*		    g e t X s i g n a l d a t a				*/
/*									*/
/*  Take X key/mouse events and turn them into Lisp events		*/
/*									*/
/************************************************************************/

extern int Current_Hot_X, Current_Hot_Y;	/* Cursor hotspot */

void
getXsignaldata(dsp)
     DspInterface dsp;
  {
    XEvent report;

    while( XPending( dsp->display_id ) )
      {
	XNextEvent( dsp->display_id, &report );
	if( report.xany.window == dsp->DisplayWindow ) /* Try the most important window first. */
	  switch(report.type) {
	  case MotionNotify :
	    *CLastUserActionCell68k = MiscStats->secondstmp;
	    *EmCursorX68K = (*((DLword *)EmMouseX68K)) = (short) ((report.xmotion.x + dsp->Vissible.x)&0xFFFF) - Current_Hot_X;
	    *EmCursorY68K = (*((DLword *)EmMouseY68K)) = (short) ((report.xmotion.y + dsp->Vissible.y)&0xFFFF) - Current_Hot_Y;
	    DoRing();
	    if(( KBDEventFlg ) > 0) Irq_Stk_End=Irq_Stk_Check=0;
	    break;
	  case KeyPress :
	    kb_trans( SUNLispKeyMap[(report.xkey.keycode)-KEYCODE_OFFSET], FALSE );
	    DoRing();
	    if((KBDEventFlg += 1) > 0) Irq_Stk_End=Irq_Stk_Check=0;
	    break;
	  case KeyRelease :
	    kb_trans( SUNLispKeyMap[(report.xkey.keycode)-KEYCODE_OFFSET], TRUE );
	    DoRing();
	    if((KBDEventFlg += 1) > 0) Irq_Stk_End=Irq_Stk_Check=0;
	    break;
	  case ButtonPress :
	    switch (report.xbutton.button) {
	    case Button1: PUTBASEBIT68K( EmRealUtilin68K, MOUSE_LEFT, FALSE );
	      break;
	    case Button2: PUTBASEBIT68K( EmRealUtilin68K, MOUSE_MIDDLE, FALSE );
	      break;
	    case Button3: PUTBASEBIT68K( EmRealUtilin68K, MOUSE_RIGHT, FALSE );
	      break;
	    default:
	      break;
	    }
	    DoRing();
	    if((KBDEventFlg += 1) > 0) Irq_Stk_End=Irq_Stk_Check=0;
	    break;
	  case ButtonRelease :
	    switch (report.xbutton.button) {
	    case Button1: PUTBASEBIT68K( EmRealUtilin68K, MOUSE_LEFT, TRUE );
	      break;
	    case Button2: PUTBASEBIT68K( EmRealUtilin68K, MOUSE_MIDDLE, TRUE);
	      break;
	    case Button3: PUTBASEBIT68K( EmRealUtilin68K, MOUSE_RIGHT, TRUE );
	      break;
	    default:
	      break;
	    }
	    DoRing();
	    if((KBDEventFlg += 1) > 0) Irq_Stk_End=Irq_Stk_Check=0;
	    break;
	  case EnterNotify :
	    Mouse_Included = TRUE;
	    break;
	  case LeaveNotify :
	    Mouse_Included = FALSE;
	    break;
	  case Expose :
	    XLOCK;
	    (dsp->bitblt_to_screen)( dsp, 0,
				    report.xexpose.x+dsp->Vissible.x,
				    report.xexpose.y+dsp->Vissible.y,
				    report.xexpose.width,
				    report.xexpose.height );
	    XUNLOCK;
	    break;
	  default:
	    break;
	  }
	else if( report.xany.window == dsp->LispWindow )
	  switch(report.xany.type) {
	  case ConfigureNotify :
	    lisp_Xconfigure(dsp, report.xconfigure.x, report.xconfigure.y,
			    report.xconfigure.width, report.xconfigure.height);
	    break;
	  case EnterNotify :
	    enable_Xkeyboard( currentdsp );
	    break;
	  case LeaveNotify :
	    break;
	  case MapNotify :
	    /* Turn the blitting to the screen on */
	    break;
	  case UnmapNotify :
	    /* Turn the blitting to the screen off */
	    break;
	  default:
	    break;
	  }
	else if ( report.xany.window == dsp->HorScrollBar )
	  switch(report.type) {
	  case ButtonPress :
	    switch( report.xbutton.button ) {
	    case Button1:
	      DefineCursor( dsp->display_id, dsp->HorScrollBar, &ScrollLeftCursor );
	      ScrollLeft( dsp, report );
	      break;	
	    case Button2:
	      DefineCursor( dsp->display_id, dsp->HorScrollBar, &HorizThumbCursor );
	      break;
	    case Button3:
	      DefineCursor( dsp->display_id, dsp->HorScrollBar, &ScrollRightCursor );
	      ScrollRight( dsp, report );
	      break;
	    default:
	      break;
	    }			/* end switch */
	    break;
	  case ButtonRelease :
	    switch( report.xbutton.button ) {
	    case Button1:
	      DefineCursor( dsp->display_id, report.xany.window, &HorizScrollCursor );
	      break;
	    case Button2:
	      JumpScrollHor( dsp, report.xbutton.x );
	      DefineCursor( dsp->display_id, report.xany.window, &HorizScrollCursor );
	      break;
	    case Button3:
	      DefineCursor( dsp->display_id, report.xany.window, &HorizScrollCursor );
	      break;
	    default:
	      break;
	    }			/* end switch */
	  default:
	    break;
	  }
	else if ( report.xany.window == dsp->VerScrollBar )
	  switch(report.type) {
	  case ButtonPress :
	    switch( report.xbutton.button ) {
	    case Button1:
	      DefineCursor( dsp->display_id, report.xany.window, &ScrollUpCursor );
	      ScrollUp( dsp );
	      break;	
	    case Button2:
	      DefineCursor( dsp->display_id, report.xany.window, &VertThumbCursor );
	      break;
	    case Button3:
	      DefineCursor( dsp->display_id, report.xany.window, &ScrollDownCursor );
	      ScrollDown( dsp );
	      break;
	    default:
	      break;
	    }			/* end switch */
	    break;
	  case ButtonRelease :
	    switch( report.xbutton.button ) {
	    case Button1:
	      DefineCursor( dsp->display_id, report.xany.window, &VertScrollCursor );
	      break;
	    case Button3:
	      DefineCursor( dsp->display_id, report.xany.window, &VertScrollCursor );
	      break;
	    case Button2:
	      JumpScrollVer( dsp, report.xbutton.y );
	      DefineCursor( dsp->display_id, report.xany.window, &VertScrollCursor );
	      break;
	    default:
	      break;
	    }			/* end switch */
	    break;
	  default:
	    break;
	  }
	else if (( report.xany.window == dsp->NEGrav ) &&
		 ( report.xany.type ==  ButtonPress) &&
		 ((report.xbutton.button & 0xFF) == Button1))
	  Set_BitGravity(report, dsp, dsp->NEGrav, NorthEastGravity);
	else if (( report.xany.window == dsp->SEGrav ) &&
		 ( report.xany.type ==  ButtonPress) &&
		 ((report.xbutton.button & 0xFF) == Button1))
	  Set_BitGravity(report, dsp, dsp->SEGrav, SouthEastGravity);
	else if (( report.xany.window == dsp->SWGrav ) &&
		 ( report.xany.type ==  ButtonPress) &&
		 ((report.xbutton.button & 0xFF) == Button1))
	  Set_BitGravity(report, dsp, dsp->SWGrav, SouthWestGravity);
	else if (( report.xany.window == dsp->NWGrav ) &&
		 ( report.xany.type ==  ButtonPress) &&
		 ((report.xbutton.button & 0xFF) == Button1))
	  Set_BitGravity(report, dsp, dsp->NWGrav, NorthWestGravity);
	XFlush(dsp->display_id);
      }				/* end while */
  } /* end getXsignaldata() */


enable_Xkeyboard( dsp )
     DspInterface dsp;
{
	XLOCK;
	XSelectInput( dsp->display_id,
		     dsp->DisplayWindow,
		     dsp->EnableEventMask );
	XFlush( dsp->display_id );
	XUNLOCK;
}



disable_Xkeyboard( dsp )
     DspInterface dsp;
{
	XLOCK;
	XSelectInput( dsp->display_id,
		     dsp->DisplayWindow,
		     dsp->DisableEventMask );
	XFlush( dsp->display_id );
	XUNLOCK;
}

beep_Xkeyboard( dsp )
     DspInterface dsp;
{
#ifdef TRACE
	printf( "TRACE: beep_Xkeyboard()\n" );
#endif

		XLOCK;
		XBell( dsp->display_id, (int) 50 );
		XFlush( dsp->display_id );
		XUNLOCK;


} /* end beep_Xkeyboard */

