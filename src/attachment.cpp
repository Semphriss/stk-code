//  $Id$
//
//  SuperTuxKart - a fun racing game with go-kart
//  Copyright (C) 2006 Joerg Henrichs
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 2
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

#include <plib/ssg.h>

#include "attachment.hpp"
#include "kart.hpp"
#include "constants.hpp"
#include "loader.hpp"
#include "world.hpp"
#include "sound_manager.hpp"

struct  initAttachmentType {attachmentType attachment; char*file;};

/* Some explanations to the attachments:
   Parachute: This will increase the air friction, reducing the maximum speed.
              It will not have too much of an effect on slow speeds, since air
              friction only becomes important at higher speeds.
   Anvil:     It increases the weight of the kart.But this will NOT have any
              effect on karts already driving at highest speed: the accelerating
	      force is independent of the mass, so it is 0 at highest speed 
	      (engine force = air- plus system-force) and only this value gets
	      divided by the mass later --> at highest speed there would be no 
	      effect when the mass is changed, only at lower speeds the acting 
	      acceleration will be lower.Reducing the power slows the kart down,
	      but doesn't give the feeling of a sudden weight increase. 
	      Therefore the anvil will reduce by a certain factor (see physics
	      parameters) once when it is attached. Together with the mass 
	      increase (lower acceleration) it's sufficient negative.
*/

initAttachmentType iat[]={
  {ATTACH_PARACHUTE,   "parachute.ac"},
#ifdef USE_MAGNET
  {ATTACH_MAGNET,      "magnet.ac"},
  {ATTACH_MAGNET_BZZT, "magnetbzzt.ac"},
#endif
  {ATTACH_ANVIL,       "anvil.ac"},
  {ATTACH_TINYTUX,     "tinytux_magnet.ac"},
  {ATTACH_MAX,         ""},

};
// -----------------------------------------------------------------------------
AttachmentManager::AttachmentManager() {
  for(int i=0; iat[i].attachment!=ATTACH_MAX; i++) {
    attachments[iat[i].attachment]=ssgLoadAC(iat[i].file, loader);
  }   // for
}   //AttachmentManager

// -----------------------------------------------------------------------------
AttachmentManager *Attachment::attachment_manager=0;

// =============================================================================
Attachment::Attachment(Kart* _kart) {
  if(!attachment_manager) {
    attachment_manager=new AttachmentManager();
  }
  type      = ATTACH_NOTHING;
  time_left = 0.0;
  kart      = _kart;
  holder    = new ssgSelector();
  kart->getModel()->addKid(holder);

  for(int i=ATTACH_PARACHUTE; i<=ATTACH_TINYTUX; i++) {
    ssgEntity *p=attachment_manager->getModel((attachmentType)i);
    holder->addKid(p);
  }
  holder->select(0);
}   // Attachmetn

// -----------------------------------------------------------------------------
Attachment::~Attachment() {
  if(attachment_manager) {
    delete attachment_manager;
    attachment_manager = 0;
  }
    ssgDeRefDelete(holder);
}   // ~Attachment

// -----------------------------------------------------------------------------
void Attachment::set(attachmentType _type, float time) {
  holder->selectStep(_type);
  type      = _type;
  time_left = time;
}   // set

// -----------------------------------------------------------------------------
void Attachment::hitGreenHerring() {
  switch (rand()%2) {
    case 0: set( ATTACH_PARACHUTE, 4.0f ) ;
            // if ( kart == kart[0] )
            //   sound -> playSfx ( SOUND_SHOOMF ) ;
            break ;
    case 1: set( ATTACH_ANVIL, 2.0f ) ;
            // if ( kart == kart[0] )
            //   sound -> playSfx ( SOUND_SHOOMF ) ;
            // Reduce speed once (see description above), all other changes are
            // handled in Kart::updatePhysics
            kart->getVelocity()->xyz[1] *= physicsParameters->anvilSpeedFactor;
            break ;
  }   // switch rand()%2
}   // hitGreenHerring

// -----------------------------------------------------------------------------
void Attachment::update(float dt, sgCoord *velocity) {
  if(type==ATTACH_NOTHING) return;
  time_left -=dt;

  switch (type) {
    case ATTACH_PARACHUTE:  // handled in Kart::updatePhysics
    case ATTACH_ANVIL:      // handled in Kart::updatePhysics
    case ATTACH_NOTHING:   // Nothing to do, but complete all cases for switch
    case ATTACH_MAX:       break;
    case ATTACH_TINYTUX:   if(time_left<=0.0) kart->handleRescue();
                           sgZeroVec3 ( velocity->xyz ) ;
                           sgZeroVec3 ( velocity->hpr ) ;
                           velocity->xyz[2] = 1.1f * GRAVITY * dt *10.0f;
                           break;
#ifdef USE_MAGNET
    case ATTACH_MAGNET:    break;
    case ATTACH_MAGNET_BZZT: float cdist; int closest;
                             kart->getClosestKart(&cdist, &closest);
			     // if no closest kart, set type to
			     // non-active magnet
			     if(closest==-1) {
			       if ( type == ATTACH_MAGNET_BZZT )
				 set( ATTACH_MAGNET, time_left ) ;
			       return;
			     }
			     // Otherwise: set type to active magnet.
			     if(type==ATTACH_MAGNET) {
			       if(kart->isPlayerKart() || closest==0) {
				 sound_manager->playSfx(SOUND_BZZT);
			       }
			       set(ATTACH_MAGNET_BZZT,
				   time_left<4.0?4.0:time_left);
			     }
			     kart->handleMagnet(cdist, closest);
			     break;
#endif
  }   // switch

  if ( time_left <= 0.0f) {
    clear();
  }   // if time_left<0
}   // update
// -----------------------------------------------------------------------------
