Notes on gameplay rules for integrating hbuilder into Harris
============================================================

Tech system
-----------

At start of each month, presented with a list of available techs,
defined by the following requirements:

* Date of tech must be at most 6 months away
* Tech's prerequisites must all have been unlocked already

Player is given three slots to fill with either a tech or a meta.

* Meta 'Supporting Research' allows one other slot to hold a tech
  from 'future' (otherwise all techs must be this month or earlier)
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

Optionality
-----------

It has to still be an option to play without the tech/builder system
and use the historical aircraft types.  But that means we will need
to either reproduce them procedurally, or re-estimate the fuel/bomb
tradeoff (to match what the doctriniser does) and write data for
`fight_factor` and `flak_factor` — assuming we're going to switch the
core game over to use separate flak and fighter defns (and two values
for the latter, too, for Schräge).

That switch-over is desirable, though, because it's already kind of
silly that having good gun turrets improves a bomber's survivability
against flak in the current Harris.

In an ideal world, we'd even have doctrine to choose between high-
and low-level flight; raids on the deck would have shorter range and
would be at risk from light flak (depending on `manu_pen`) but heavy
flak wouldn't be able to hit and fighters would have a difficult time
spotting you (neither ground based nor airborne radar can pick you
up, you won't be caught in searchlights, nor even silhouetted against
a lit-up cloud etc.); also navigation might be more precise, at least
on moonlit nights.  Unfortunately, attempts to make altitude affect
range didn't go so well (can't balance the Whitley properly) so we
might have to do without that piece.
