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
  - maybe it should remove the date-lock on whatever tech you were
    using it to support?
* Mothball Labs: will increase the budget (cshr)
  - by 4800 in 1939, plus 400 each year?
* Production Engineering: will increase pc of that type
  - by 15%?

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
might have to do without that piece.  (We could just cop out by
reducing range by a fixed factor when flying low, with the variable
effect residing in things like `manu_pen` vs. flak `evade_factor` and
low-level map-reading (requires high crew skill) vs. navaids at
20,000ft.)

Developing a four-engined heavy from scratch costs around a million
funds; a typical mark upgrade is about 150k.  (A minor mark upgrade
for a medium bomber (say, Wellington Ic) is more like 55k, a major
mark (like the III) nearer 80k, a mod maybe 25-30k.)  Thus a typical
roughly-like-historical playthrough would spend something like four
million funds on prototyping and tooling over the first four years of
the war; that's the equivalent of 100 Lancasters.  So there needs to
be some extra budget available to make up for that; a very rough
estimate suggests 10% extra funding just about covers it.

Legacy Fleet
------------

In the standard game, you start with 123 BLEN, 147 WHIT (of which 50
are on OTUs), 83 HAMP and 73 WLNG; a total of 426 a/c (376 in active
squadrons) with a combined build cost of just over 3.5 million funds.
The approximate cost and time to develop each of these types under
the builder system would be:
* Blenheim IV: 250k, 106 days
* Whitley IV: 450k, 166 days
  - Whitley V: 60k, 107 days
* Hampden: 410k, 135 days
* Wellington Ia: 560k, 151 days
When playing with the builder, first you design your 'rearmament'
types, with y=0 tech only (these correspond to Blenheims and Whitley
IIIs or maybe IVs).  You get 2.25M funds to tool and build these a/c;
anything left unspent is wasted.
Then the y=2 techs unlock, allowing you to design 'legacy' types (and
improved marks/mods of your 'rearmament' types).  You get another 3M
funds to tool and build.  Initial production caps for these types are
lower than for the 'rearmament' types.
Then you create squadrons for your various types; bombers (except any
you assign to Training Units) will be distributed roughly evenly
across all squadrons using that type, then crews will be assigned by
the usual method.
