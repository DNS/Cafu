Carried Weapons Overview     {#CarriedWeaponsOverview}
========================

The weapons that a player can carry (GameSys::ComponentCarriedWeaponT components) and the player's
1st-person model (GameSys::ComponentModelT component) are closely interwoven.

This page provides an overview of how these components work together.


Animation Sequence is State
---------------------------

In the DeathMatch sample game implementation, we have chosen to consider the animation sequence
that the 1st-person weapon model is currently playing also as the current "state" of the human player.

That is, for example, if we're currently playing a "firing" sequence, then the player *is* firing --
there is no other or explicit record that that player is currently in "firing" state.

This also means that we depend on monitoring the wraps or ends of the 1st-person weapon model animation
sequences, because when an end is reached, we may want to re-evaluate the entity's state.
For example, if the firing sequence has reached its end, should we
  - loop in order to continue firing (e.g. if the player is still holding fire button down),
  - switch back to idle (e.g. if the fire button is no longer down),
  - or even switch to reload (e.g. if all the ammo is spent)?

This monitoring of sequence ends and wraps is achieved by weapon-specific implementations of the
1st-person weapon models GameSys::ComponentModelT::OnSequenceWrap_Sv() callback method.

Note that this is specific to the weapon handling only, and *not* applicable to other parts of the player's state.
That is, for example, whether the player is standing, walking or running is *not* defined by the player body's
animation sequence, but rather by explicit records of its velocity and other data.


Idle is central
---------------

In the DeathMatch sample game implementation, in order to reduce interactions among animation sequences,
we have at this time also chosen to start new weapon activities, such as "firing", "reloading" or "holstering",
only when the weapon is in "idle" state.

Whether a weapon is in "idle" state is defined by its GameSys::ComponentCarriedWeaponT::IsIdle() implementation, which usually returns `true`
if one of the "idle" sequences of the 1st-person weapon model is currently playing.

The key idea is that a weapon that is in "idle" state can be interrupted for anything else at any time
(smoothly blending to the new activity if required), but if some activity is already going on ("not idle"),
the weapon must return to "idle" before another activity can begin.

Note that this still allows to interrupt activities that are composed of their own series of animation sequences.
For example, if a "reloading" activity:
  - begins with an animation sequence for opening the bullets compartment,
  - is followed by a sequence for inserting a new bullet,
  - the insertion sequence is looped until the desired number of bullets has been inserted,
  - is concluded by a final sequence that snaps shut the bullets compartment,
then this activity can be interrupted by breaking the loop of the insertion sequence. In fact, the implementation
might support interrupting the above activity at any time. A very quick interruption might even blend smoothly
from opening to immediately closing the bullets compartment again.
But in any case, the weapon must be led back to "idle" state before it can begin another (different) activity,
such as "firing" or "holstering".

This choice of organizing model activities eliminates a large number of very complex problems that would otherwise
occur if we tried to model fully arbitrary transitions.


Picking up new weapons
----------------------

When a weapon is newly picked up, these steps occur (in the DeathMatch sample game implementation):

  1. In each server frame, the human player's GameSys::ComponentCollisionModelT component determines if it finds
     itself intersecting another entity's trigger volume. It calls the other entity's
     GameSys::ComponentScriptT::OnTrigger() callback if positive.

  2. If the other entity is a weapon waiting to be picked up, its GameSys::ComponentScriptT::OnTrigger() implementation in
     `Weapon.lua` forwards the call back to the original entity (the human player), calling its method
     GameSys::ComponentScriptT::PickUpItem().

  3. The human player's implementation of GameSys::ComponentScriptT::PickUpItem() finds the matching weapon in the
     player entity's list of GameSys::ComponentCarriedWeaponT components, and calls its GameSys::ComponentCarriedWeaponT::PickedUp() method in turn.

  4. GameSys::ComponentCarriedWeaponT::PickedUp() marks the weapon as available to the player and (re-)stock the related
     ammo in the weapon and the player's inventory.

This completes the picking up of a weapon. If the weapon was newly picked up (not available to the player before),
GameSys::ComponentCarriedWeaponT::PickedUp() also calls GameSys::ComponentHumanPlayerT::SelectWeapon() in order to activate the weapon.


Selecting weapons
-----------------

When a weapon is newly picked up or the player presses a key for selecting a previously picked up weapon,
these steps occur (in the DeathMatch sample game implementation):

  1. GameSys::ComponentHumanPlayerT::SelectWeapon() initiates the holstering of the currently still active weapon and the
     subsequent drawing of the new weapon:

       - If the previous (still current) weapon is unknown or not available to the player (has never been picked up),
         the holstering is skipped and the next weapon is drawn immediately as described below.

       - If the current weapon is fine, its GameSys::ComponentCarriedWeaponT::IsIdle() method is called in order to learn whether the weapon can be
         changed at all. If the weapon is not idle at the time that this method is called (e.g. reloading or
         firing), all subsequent steps are *ignored*, that is, the weapon is *not* changed.

       - If it turns out that the current weapon does not support holstering (e.g. because there is no holstering
         sequence available), GameSys::ComponentHumanPlayerT::SelectNextWeapon() is called immediately.

       - Otherwise (the normal case), GameSys::ComponentCarriedWeaponT::Holster() is called.

  2. The GameSys::ComponentCarriedWeaponT::Holster() implementation does nothing but initiate the holster sequence as appropriate for the weapon.
     Note that this requires that the GameSys::ComponentModelT::OnSequenceWrap_Sv() callback has previously been properly setup to handle
     the next step.

  3. The current 1st-person weapon model's GameSys::ComponentModelT::OnSequenceWrap_Sv() callback is automatically called when
     the holster sequence has reached its end.
     Its implementation calls GameSys::ComponentHumanPlayerT::SelectNextWeapon() in order to activate the next weapon.

  4. GameSys::ComponentHumanPlayerT::SelectNextWeapon() calls the GameSys::ComponentCarriedWeaponT::Draw() method of the newly activated weapon,
     which sets up the entity's 1st-person weapon model for drawing the new method. It also updates the
     models's GameSys::ComponentModelT::OnSequenceWrap_Sv() callback.

  5. When the end of the draw sequence has been reached, the newly setup GameSys::ComponentModelT::OnSequenceWrap_Sv() callback leads
     the weapon to idle state (sets an "idle" animation sequence).

This completes the selection of another weapon.


Also see
--------

  - GameSys::ComponentCarriedWeaponT
  - GameSys::ComponentHumanPlayerT
  - GameSys::ComponentModelT
