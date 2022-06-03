Notes on gameplay rules for integrating hbuilder into Harris
============================================================

Tech system
-----------

At start of each month, presented with a list of available techs,
defined by the following requirements:

* Year of tech must be at most 6 months away
* Tech's prerequisites must all have been unlocked already

Player is given three slots to fill with either a tech or a meta.

* Meta 'Supporting Research' allows one other slot to hold a tech
  from 'next year' (otherwise all techs must be this year or earlier)
* Meta 'Mothball Labs'
* For each Type in production, meta 'Production Engineering'

At end of month, one of the three slots will be picked at random to
have its effect applied, as follows:

* A tech: will be unlocked
* Supporting Research: will do nothing
* Mothball Labs: will increase the budget (cshr)
* Production Engineering: will increase pc of that type

Prototyping
-----------

The design calculations aren't perfectly reliable; they're just a
prediction.  The true performance of an aeroplane is only known
after the prototype has been built and tested.  When the proto is
complete, randomisation factors are applied to certain design
outputs, and subsequent outputs recalculated:

* Drag: up to ±5% (FRESH) / ±2% (MARK)
* Serv: up to ±4 (FRESH) / ±3 (MARK) / ±2 (MOD)
* Vuln: up to ±10% (FRESH) / ±5% (MARK) / ±2% (MOD)
* Manu: up to ±5% (FRESH) / ±2% (MARK)
* Accu: up to ±4 (FRESH) / ±1 (MARK)

Note that refits cannot take the total bonus/malus on any axis
outside the range that would be possible for a fresh design.  Thus
if e.g. the original design had a +5% drag modifier, a mark refit
could have a drag modifier anywhere from +3% to +5%, but if +6% or
+7% were rolled it would be truncated to +5%.

Prototyping takes time (and money), and so does tooling up for
series production.  You can either order a design 'off the drawing
board', running both processes in parallel, or prototype the design
first, waiting to see whether the bomber is a lemon before committing
to the tooling.  That might save you a lot of money on designs that
don't come out well, but the ones that you do proceed with will
arrive much later (and be that much more outdated when they do).
For example, a reasonable approximation of a Lancaster Mk I as a
clean-sheet design has:
```
Prototype in 226 days for 544728 funds
Production in 410 days for 1089457 funds
```
So to avoid the risk of wasting a million on a bad design, you would
have to delay series production by over seven months!

Note that each manufacturer can only have one design prototyping and
one design (not necessarily the same one) tooling for production at
any point in time.  This includes Mark and Mod designs, though those
are far quicker and cheaper to develop; you may (for instance)
temporarily pause work on a Fresh design to quickly run through a
refit of an old bomber, before resuming the fresh design from where
you left off.
