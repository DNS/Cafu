.. _dependencies_among_models:

Dependencies among Models
=========================

Although it might not be obvious at a first glance, there are several
dependencies among certain models. It is worthwhile to have knowledge
about these issues before you start making own models, because it helps
with resource planning and prevents expensive problems later.

We'll describe several typical kinds of dependencies, considering the
example of the Cafu DeathMatch MOD: In the Cafu DeathMatch MOD, mutual
dependencies affect the human player models and their weapons.

First, lets deal with all “other” models, i.e. those that are neither
player nor weapon models: Usually, these other models are not closely
related to each other, each of them has a separate piece of game code
associated that handles it, and thus they do not suffer from any
inherent dependency problems.

Human player models
-------------------

Human player models are special, because they are usually intended to be
100% equivalent to each other. That is, if somebody makes a new human
player model and offers it for download, you expect it to work exactly
like the ones that you already know. In order to achieve this kind of
equivalence, two assumptions must hold: The skeleton of the new models
must basically match the skeleton of the old models, and the animation
sequence numbers must refer to reasonably identical animations.

The animation sequences must match, because the game code has built-in
knowledge that, for example, sequence number 3 refers to an “idle
(waiting)” animation, and that sequence number 27 refers to an “aiming
with a shotgun” animation. It is entirely up to you to animate your
model to look at his wrist watch while aiming with the shotgun, or to
pick his nose, but it *is* important that sequence number 27 corresponds
to some “aiming with a shotgun” animation – because the same is true for
all other models, and the game code relies on it.

The skeletons must also match, for similar reasons: At least the basic
hierarchical structure (starting from the pelvis to the most important
bones) must be identical, as well as the *names*\ (!) of the
corresponding bones. However, you are free to add bones to the skeleton
as you like, change their sizes or lengths, change their positions
relative to each other, and do many other interesting things. You may
even omit bones if you want to create a one-armed, one-legged hero. What
works and what works not is easiest determined by trying it out, but
please do also refer to the next part about weapons.

Weapons
-------

Weapons do usually come as a set of three models: “world” models,
“player” models, and “view” models.

**World** models are the models that lie around in the world, before
someone picked them up. They are usually not animated (or only have a
single animation sequence), are independent from anything else, and are
therefore in the same category as the “all others” models, so that we
need not be further concerned about them.

**Player** models are the weapons that you see in the hands of *other*
players who have picked up and are using that weapon. For the following
discussion, we'll refer to the “player” weapon model as the “\ ``_p``\ ”
model, and to the character model of a human player as the “body” model.

First, if you consider the skeleton of a ``_p`` model in the Model
Editor, you will find that it resembles a partial body model (the bones
from the pelvis to the shooting arm are there!), before it diverges into
additional bones for the actual weapon.

Here is the crucial point: In order for the engine to compute the proper
position of the ``_p`` model relative to the body model, it (partially)
has to match the skeletons of both models! That is, it first computes
the skeleton of the body model (depending on its current animation
pose). Then it considers the skeleton of the ``_p`` model, starting at
its root, and tries to match it bone-by-bone to the previously computed
body skeleton. If a match was found, the engine simply takes the
information from the body model bone also for the ``_p`` model bone.
Only when the matching breaks for the first time, the engine resumes
normal bone computing also for the ``_p`` model. (This way you can for
example see a face-hugger in the hands of *another* player that is
wagging it's tail.) Matches are made by comparing the *names* of the
concerned bones.

As a consequence, if you want to make additional body models for the
DeathMatch MOD, and additional weapons, and you want to be able to
combine each body model with each weapon, *then you have to make sure
that they*\ **all**\ *have a corresponding skeletal structure and bone
names!*

**View** models are the models that you see in 1st persons view after
you have picked up a weapon yourself. They are also independent from
anything else, but the game has usually special code for handling them.
Thus, you can well make a *replacement* weapon for e.g. the shotgun,
matching the animation sequences of the existing “view” weapon model
according to similar rules as indicated for making replacement human
player models. You can also introduce *entirely new* weapon models, but
be prepared that is requires to augment the game's C++ or script code as
well.

Applicability to your own game
------------------------------

If you create an own game, things may or may not be different, of
course. However, keep in mind that if you want to achieve a high degree
of flexibility and ease of maintenance, you'll sooner or later probably
experience the same rules and dependencies as described here. They are
the – relatively cheap – price for the ability to combine every human
player model with every weapon model.
