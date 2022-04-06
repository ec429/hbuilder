#!/usr/bin/python
# encoding: utf-8
import enum
import math

kg = 2.2 # lbs

class Manu(enum.Enum):
    # Each manufacturer gets specified bonuses/maluses
    ARMSTRONG = 0
    AVRO = 1
    BRISTOL = 2
    DH = 3
    HP = 4
    SHORTS = 5
    SUPERMARINE = 6
    VICKERS = 7

class Engine(object):
    def __init__(self, manu, name, power, vuln, rely, serv, cost, sc, tare, drag, desc):
        self.manu = manu
        self.name = name
        self.power = power # hp
        self.vuln = vuln / 100.0 # nominally, % chance a hit will disable the engine
        self.rely = rely / 100.0 # nominally, % chance engine will fail on a long flight
        self.serv = serv / 100.0 # nominally, % chance engine will need maintainence on a given day
        self.cost = cost # pounds sterling 1940s (the basic currency unit in Harris)
        self.sc = sc # 3 levels: single-speed, two-speed, two-stage.
        self.tare = tare # lb
        self.drag = drag # lbf
        self.desc = desc
    def power_at(self, alt, fs=False):
        fth = 10.25
        scp = 0
        if self.sc == 2:
            if fs:
                fth = 16
                scp = 0.02 # power cost of running SC in high (F.S.) gear
        elif self.sc == 3:
            fth = 12.0
            if fs:
                fth = 21.0
                scp = 0.06 # power cost of running SC in high (F.S.) gear
        elif fs:
            return 0.0
        density = math.exp(min(fth-alt, 0)/25.1)
        return self.power * (density - scp)
    def power_at_alt(self, alt):
        # altitude in 1000s of ft
        return max(self.power_at(alt, False), self.power_at(alt, True))
    @property
    def fuelrate(self):
        return self.power * 0.36 # SFC in lb/hp·h.  Merc VI has 0.49, Merlin XX has 0.52, cruise is below full power
    def manumatch(self, m):
        if self.manu == 'Bristol':
            return m == Manu.BRISTOL
        if self.manu == 'Armstrong-Siddeley':
            return m == Manu.ARMSTRONG
        return False

ENG_VULN_LIQUID_COOLED = 10
ENG_VULN_AIR_COOLED = 6

engines = [Engine('Bristol', 'Mercury XV', 840, ENG_VULN_AIR_COOLED, 0.8, 0.8, 800, 1, 440 * kg, 90, "1938 developed Mercury radial, as used on Blenheim IV."), # wiki says 920hp in Blenheim article but 825 in Mercury article
           Engine('Rolls-Royce', 'Merlin IV', 1030, ENG_VULN_LIQUID_COOLED, 0.6, 1, 1000, 1, 600 * kg, 72, "Early glycol-cooled Merlin as used on Whitley Mk IV."),
           Engine('Rolls-Royce', 'Merlin X', 1145, ENG_VULN_LIQUID_COOLED, 0.5, 1, 1500, 2, 660 * kg, 65, "Improved early Merlin with two-speed blower and water/glycol cooling.  Powered the Whitley V as well as Wellington II and Halifax I."),
           Engine('Rolls-Royce', 'Merlin XX', 1240, ENG_VULN_LIQUID_COOLED, 0.5, 0.9, 1600, 2, 680 * kg, 70, "Mid-period Merlin, most famously used on the Lancaster."),
           Engine('Rolls-Royce', 'Merlin 60', 1565, ENG_VULN_LIQUID_COOLED, 0.5, 1.2, 2000, 3, 750 * kg, 105, "Two-stage supercharged Merlin, for high altitude use (e.g. Wellington VI, Mosquito IX)."),
           Engine('Bristol', 'Pegasus XVIII', 965, ENG_VULN_AIR_COOLED, 1.0, 0.8, 1200, 2, 1111, 84, "Supercharged radial developed from the Jupiter.  As used on Hampden and Wellington I."),
           Engine('Bristol', 'Hercules XI', 1500, ENG_VULN_AIR_COOLED, 0.9, 0.7, 1600, 1, 1929, 102, "14-cylinder two-row sleeve-valve radial."),
           Engine('Rolls-Royce', 'Vulture I', 1760, ENG_VULN_LIQUID_COOLED, 3.0, 1.5, 1800, 2, 2450, 90, "Troubled X-24 engine as used on Manchester Mk I."),
           Engine('Bristol', 'Hercules III', 1375, ENG_VULN_AIR_COOLED, 1.2, 0.9, 1400, 1, 1850, 104, "Early-model 14-cylinder two-row sleeve-valve radial."),
           Engine('Bristol', 'Hercules XVIII', 1675, ENG_VULN_AIR_COOLED, 0.8, 0.8, 2100, 1, 1940, 101, "Developed 14-cylinder two-row sleeve-valve radial."),
           Engine('Armstrong-Siddeley', 'Deerhound II', 1500, ENG_VULN_AIR_COOLED, 0.8, 0.8, 1200, 2, 1600, 93, "Triple-row 21-cylinder radial."), # made-up tare
           Engine('Armstrong-Siddeley', 'Tiger VIII', 920, ENG_VULN_AIR_COOLED, 0.7, 0.6, 800, 2, 1287, 72, "14-cylinder two-row radial."), # made-up tare
           ]

class CoverageDirection(enum.Enum):
    FRONT = 1
    BEAM_HIGH = 2
    BEAM_LOW = 3
    TAIL_HIGH = 4
    TAIL_LOW = 5
    BENEATH = 6
    @property
    def high(self):
        return self in (self.BEAM_HIGH, self.TAIL_HIGH)
    @property
    def low(self):
        return self in (self.BEAM_LOW, self.TAIL_LOW, self.BENEATH)

class GunMount(enum.Enum):
    NOSE = 1 # also conflicts with Engines.odd
    DORSAL = 2
    TAIL = 3
    WAIST = 4
    VENTRAL = 5
    CHIN = 6

class Guns(object):
    name = None
    desc = None
    serv = 0.5 # nominally, % chance turret will need maintainence on a given day
    tare = None
    drag = 0 # nominally mph to knock off speed.  We'll need some fudge-factor to convert this to force for the power/climb calculator
    loxn = None # which GunMount slot does this occupy?
    guns = 2 # Number of guns in .303s equivalent (for ammomass calc)
    @property
    def ammomass(self):
        # I think a .303 bullet + cartridge is about 0.09lb
        # and we'll assume 2000rpg
        return self.guns * 180
    def coverage(self, direction):
        # Return firepower in .303s equivalent
        raise NotImplementedError()

class NoseGun(Guns):
    name = 'Nose gun (single)'
    desc = 'Single .303 Vickers K or Browning in a nose turret or flexible mount.'
    serv = 0.2
    tare = 25 # made-up figure
    drag = 4
    loxn = GunMount.NOSE
    guns = 1
    def coverage(self, direction):
        if direction == CoverageDirection.FRONT:
            return 1
        return 0
class Nose(Guns):
    name = 'Nose turret (twin)'
    desc = 'Twin .303 Vickers K or Brownings in a nose turret.'
    tare = 100 # made-up figure
    drag = 6
    loxn = GunMount.NOSE
    def coverage(self, direction):
        if direction in (CoverageDirection.FRONT, CoverageDirection.BEAM_HIGH):
            return 2
        return 0
class Dorsal(Guns):
    name = 'Dorsal turret (twin)'
    desc = 'Twin .303 Brownings in a fully rotating mid-upper turret, like FN.50.'
    tare = 100 # made-up figure
    drag = 10
    loxn = GunMount.DORSAL
    def coverage(self, direction):
        if direction.high:
            return 2
        return 0
class DorsalAft(Guns):
    name = 'Dorsal after mount'
    desc = 'Twin .303 Brownings in a rearward-facing upper blister mount.  As used on Hampden.'
    tare = 30 # made-up figure
    drag = 6
    loxn = GunMount.DORSAL
    def coverage(self, direction):
        if direction == CoverageDirection.TAIL_HIGH:
            return 2
        return 0
class DorsalQuad(Guns):
    name = 'Dorsal turret (quad)'
    desc = 'Quad .303 Brownings in a fully rotating mid-upper turret.  As used on Halifax I.'
    tare = 170
    drag = 12
    loxn = GunMount.DORSAL
    guns = 4
    def coverage(self, direction):
        if direction == CoverageDirection.FRONT:
            return 2 # limited field of fire
        if direction.high:
            return 4
        return 0
class Ventral(Guns):
    name = 'Ventral "dustbin"'
    desc = 'Retractable mid-under turret with twin .303 Brownings, like FN.17.'
    serv = 1.0
    tare = 170
    drag = 20
    loxn = GunMount.VENTRAL
    def coverage(self, direction):
        if direction.low:
            return 1.5 # poor sighting
        return 0
# Improved ventral (FN.21): tare = 120.  Later tech
class VentralAft(Guns):
    name = 'Ventral after mount'
    desc = 'Twin .303 Brownings in a rearward-facing under mount.  As used on Hampden.'
    tare = 50 # made-up figure
    drag = 8
    loxn = GunMount.VENTRAL
    def coverage(self, direction):
        if direction in (CoverageDirection.TAIL_LOW, CoverageDirection.BENEATH):
            return 2
        return 0
class Waist(Guns):
    name = 'Waist/Beam mount'
    desc = '.303 Brownings in the waist, one on each side.'
    tare = 30 # made-up figure
    drag = 3
    loxn = GunMount.WAIST
    def coverage(self, direction):
        if direction in (CoverageDirection.BEAM_HIGH, CoverageDirection.BEAM_LOW):
            return 1
        return 0
class Chin(Guns):
    name = 'Rearward chin mount'
    desc = 'Twin .303 Brownings in a rearward-facing mount under the nose.  As used on Blenheim.'
    tare = 50 # made-up figure
    drag = 5
    loxn = GunMount.CHIN
    def coverage(self, direction):
        if direction == CoverageDirection.TAIL_LOW:
            return 1.5 # poor sighting
        return 0
class Tail(Guns):
    name = 'Rear turret (twin)'
    desc = 'Twin .303 Brownings in a tail turret.'
    tare = 120 # made-up figure
    drag = 7
    loxn = GunMount.TAIL
    def coverage(self, direction):
        if direction in (CoverageDirection.TAIL_LOW, CoverageDirection.TAIL_HIGH):
            return 2
        return 0
class TailQuad(Guns):
    name = 'Rear turret (quad)'
    desc = 'Quad .303 Brownings in a tail turret.'
    tare = 190
    drag = 7
    loxn = GunMount.TAIL
    guns = 4
    def coverage(self, direction):
        if direction in (CoverageDirection.TAIL_LOW, CoverageDirection.TAIL_HIGH):
            return 4
        return 0

turrets = [NoseGun(), Nose(), Dorsal(), DorsalAft(), DorsalQuad(), Ventral(), VentralAft(), Waist(), Chin(), Tail(), TailQuad()]
class Engines(object):
    def __init__(self, number, typ):
        self.number = number
        self.typ = typ
        self.manu = None
    @property
    def odd(self):
        return bool(self.number % 2)
    @property
    def power_factor(self):
        num = float(self.number)
        if self.odd:
            num -= 0.1 # lose some power to slipstream drag on fuselage
        return num
    @property
    def vuln(self):
        return self.typ.vuln
    @property
    def rely1(self):
        # failure but we can get home if we have more than 2 engines
        return 1.0 - math.pow(1.0 - self.typ.rely, self.number)
    @property
    def rely2(self):
        # P(2 or more failures) = 1 - P(0 failures) - P(exactly 1 failure)
        return 1.0 - math.pow(1.0 - self.typ.rely, self.number) - self.number * self.typ.rely * math.pow(1.0 - self.typ.rely, self.number - 1)
    @property
    def serv(self):
        # P(at least one engine unserviceable)
        return 1.0 - math.pow(1.0 - self.typ.serv, self.number)
    @property
    def cost(self):
        return self.number * self.typ.cost * 1.5
    @property
    def sc(self):
        return self.typ.sc
    @property
    def fuelrate(self):
        return self.number * self.typ.fuelrate
    @property
    def tare(self):
        mounts = 100.0 * kg * self.number
        if self.odd:
            mounts -= 25.0 * kg # nose mounts are lighter than nacelles
        mounts += 60.0 * kg * int(self.number / 2) # wing structure to hold 4 engines
        if self.typ.manumatch(self.manu):
            mounts *= 0.8
        return self.typ.tare * self.number + mounts
    @property
    def drag(self):
        return self.typ.drag * self.number
    def power_at_alt(self, alt):
        return self.power_factor * self.typ.power_at_alt(alt)

# Wing: Cl and Cd curves; aoa -> lift -> speed -> drag (+ parasitic) -> power ? then minimise power (for climb)? or solve for say 80% power for cruise
# alternatively, assume constant L/D (fn of aspect ratio), set lift=weight, find drag=thrust, then relate speed to power; but need to know min. speed at which the required lift is achievable, hence max Cl
# some bogus formulae: Clmax = pi²/6(1 + 2/AR); L/D = pi*AR^0.5 (very bogus; cruising AoA, not that of Clmax)

class Wing(object):
    def __init__(self, area, span):
        self.area = float(area) # sq ft
        self.span = float(span) # ft
        self.manu = None
    @property
    def chord(self):
        return self.area / self.span # ft
    @property
    def ar(self):
        return self.span * self.span / self.area
    @property
    def cl(self):
        return math.pi * math.pi / 6.0 / (1.0 + 2.0 / self.ar)
    @property
    def ld(self):
        mf = 1.05 if self.manu == Manu.SUPERMARINE else 1.0
        return math.pi * math.sqrt(self.ar) * mf
    def lift(self, v):
        # L = ½Cl rho v^2 S
        # rho is about 1.2kg/cu m = 0.075lbm/cu ft ~= 0.0075lbf/cu ft (take g ~= 10)
        # with v in mph
        u = v * 22 / 15.0 # ft/s
        return self.cl * 0.0075 * u * u * self.area / 2.0 # lb
    def minv(self, wt, alt): # minimum speed for lift of wt lb at alt 000 ft
        # wt = L = ½Clmax ρv²S
        # 2wt/(Clmax ρS) = v²
        density = 0.0075 * math.exp(-alt/25.1) # lbf/cu ft
        u = math.sqrt(wt * 2.0 / (self.cl * density * self.area)) # ft/s
        return u * 15.0 / 22.0 # mph
    @property
    def tare(self):
        # high aspect ratios are heavy
        mar = {Manu.ARMSTRONG: 6.0}.get(self.manu, 8.0)
        arpen = math.sqrt(max(self.ar, mar)) / 6.0
        return math.pow(self.span, 1.5) * math.pow(self.chord, 1.2) * arpen

class Crewpos(enum.Enum):
    P = 1
    N = 2
    B = 3
    W = 4
    E = 5
    G = 6

class Crewman(object):
    def __init__(self, pos, gun=False):
        self.pos = pos
        self.gun = gun
    @property
    def gunner(self):
        return self.pos == Crewpos.G or self.gun
Pilot = Crewman(Crewpos.P)
Nav = Crewman(Crewpos.N)
BombAimer = Crewman(Crewpos.B)
WirelessOp = Crewman(Crewpos.W)
Engineer = Crewman(Crewpos.E)
Gunner = Crewman(Crewpos.G)

class BBGirth(enum.Enum):
    SMALL = 1 # 500lb (GP) only
    MEDIUM = 2 # 4,000lb (HC) with drag cost
    COOKIE = 3 # 4,000lb fully enclosed

class BombBay(object):
    def __init__(self, cap, girth):
        self.cap = cap # lb HE
        self.girth = girth
        self.manu = None
    @property
    def tare(self):
        factor = {BBGirth.SMALL: 0.07,
                  BBGirth.MEDIUM: 0.09,
                  BBGirth.COOKIE: 0.11}[self.girth]
        if self.girth == BBGirth.COOKIE and self.manu == Manu.AVRO:
            factor *= 0.92
        big = 9000 if self.manu == Manu.AVRO else 8000
        if self.cap > big:
            factor += (self.cap - big) / 3e5
        return self.cap * factor

class FuseType(enum.Enum):
    NORMAL = 0
    SLENDER = 1
    SLABBY = 2
    GEODETIC = 3

class Electrics(enum.Enum):
    LOW = 0 # minor utilities only
    HIGH = 1 # can drive GEE
    STABLE = 2 # OBOE, H2S, GH

class Bomber(object):
    def __init__(self, engines, turrets, wing, crew, bay, fuel, manu, ftype=FuseType.NORMAL, elec=Electrics.HIGH, haddock=False, sst=True):
        self.engines = engines
        self.engines.manu = manu
        self.turrets = turrets
        self.wing = wing
        self.crew = crew
        self.bay = bay
        self.bay.manu = manu
        self.fuel = fuel # hours
        self.manu = manu
        self.wing.manu = manu
        self.ftype = ftype
        self.elec = elec
        self.haddock = haddock # high altitude doctrine
        self.sst = sst
        e, w = self.validate()
        if e:
            raise Exception(e)
    @property
    def gun_conflict(self):
        slots = set()
        if self.engines.odd:
            slots.add(GunMount.NOSE)
        for turret in self.turrets:
            if turret.loxn in slots:
                return turret.loxn
            slots.add(turret.loxn)
        return None
    @property
    def need_gunners(self):
        return len(self.turrets)
    @property
    def gunners(self):
        return len([c for c in self.crew if c.gunner])
    def validate(self):
        errors = []
        warnings = []
        if self.gun_conflict:
            errors.append("Conflicting gun turrets in position %s" % (self.gun_conflict,))
        if self.need_gunners > self.gunners:
            warnings.append("More turrets than gunners; defence will be impaired")
        if self.ftype == FuseType.GEODETIC and self.manu != Manu.VICKERS:
            errors.append("Geodetic structures can only be built at Vickers")
        return errors, warnings
    @property
    def gundrag(self):
        # fudge factor conversion
        return sum(g.drag for g in self.turrets) * 24.0
    @property
    def guntare(self):
        return sum(g.tare for g in self.turrets) * 1.75 # account for mounting structure
    @property
    def gunammo(self):
        return sum(g.ammomass for g in self.turrets)
    @property
    def gunserv(self):
        return sum(g.serv / 100.0 for g in self.turrets)
    def rate_guns(self, schrage):
        de = 0
        for direction in CoverageDirection:
            if direction == CoverageDirection.BENEATH and not schrage:
                continue
            cov = sum(turret.coverage(direction) for turret in self.turrets)
            raw = {CoverageDirection.FRONT: 1,
                   CoverageDirection.BEAM_HIGH: 2,
                   CoverageDirection.BEAM_LOW: 1,
                   CoverageDirection.TAIL_HIGH: 2,
                   CoverageDirection.TAIL_LOW: 3,
                   CoverageDirection.BENEATH: 3}[direction]
            de += raw / (1.0 + cov)
        return de * (self.need_gunners + 1.0) / (self.gunners + 1.0)
    @property
    def rely1(self):
        return self.engines.rely1
    @property
    def rely2(self):
        return self.engines.rely2
    @property
    def serv(self):
        total = self.engines.serv * 6.0
        total += self.gunserv
        total += {FuseType.NORMAL: 3.0,
                  FuseType.SLENDER: 6.4,
                  FuseType.SLABBY: 2.7,
                  FuseType.GEODETIC: 2.0}[self.ftype] / 100.0
        if self.manu == Manu.SHORTS:
            total += 0.15
        return 1.0 - total
    @property
    def fail(self):
        total = self.rely1 * 2.0 + self.rely2 * 30.0
        total += {FuseType.NORMAL: 3.0,
                  FuseType.SLENDER: 4.0,
                  FuseType.SLABBY: 2.7,
                  FuseType.GEODETIC: 2.0}[self.ftype] / 100.0
        return total
    @property
    def fuelmass(self):
        return self.engines.fuelrate * self.fuel
    @property
    def core_tare(self):
        total = self.guntare
        # each crewman incurs a bunch of incidentals
        total += 75 * kg * len(self.crew)
        total += self.bay.tare
        return total * (1.08 if self.manu == Manu.SHORTS else 1.0)
    @property
    def fuse_tare(self):
        core = self.core_tare
        return core * {FuseType.NORMAL: 1.65 if self.manu == Manu.DH else 1.5,
                       FuseType.SLENDER: 1.0,
                       FuseType.SLABBY: 0.81 if self.manu == Manu.ARMSTRONG else 0.9,
                       FuseType.GEODETIC: 1.7}[self.ftype]
    @property
    def fuel_tare(self):
        return self.fuelmass * 0.12 * (1.1 if self.sst else 1.0)
    @property
    def tare(self):
        total = self.core_tare
        # account for fuselage/empennage tare
        total += self.fuse_tare
        total += self.fuel_tare # fuel tankage mass
        total += self.wing.tare * 1.35 # account for fuselage structure & tailplane
        total += self.engines.tare * 1.5 # account for incidentals
        return total
    @property
    def gross(self):
        total = self.tare
        total += self.fuelmass * 0.6 # typical with full bomb load (?)
        total += self.gunammo
        total += 80 * kg * len(self.crew)
        total += self.bay.cap # assumes full bomb load
        return total
    @property
    def wing_drag(self):
        return self.gross / self.wing.ld # L = W
    @property
    def fuse_drag(self):
        mf = 0.9 if self.manu == Manu.SUPERMARINE else 1.0
        # very rough approximation
        return math.pow(self.tare - self.wing.tare, 0.5) * mf * {
            FuseType.NORMAL: 4.5 if self.manu == Manu.DH else 5.0,
            FuseType.SLENDER: 3.0,
            FuseType.SLABBY: 7.2,
            FuseType.GEODETIC: 5.4}[self.ftype]
    @property
    def drag(self):
        return self.wing_drag + self.fuse_drag + self.engines.drag + self.gundrag
    @property
    def guncost(self):
        return sum(3 * turret.tare + 10 * turret.guns for turret in self.turrets)
    @property
    def core_cost(self):
        mf = {Manu.SHORTS: 0.8, Manu.AVRO: 1.05}.get(self.manu, 1.0)
        tf = {FuseType.SLENDER: 1.6,
              FuseType.SLABBY: 0.8}.get(self.ftype, 1.0)
        ef = 2.0 if self.engines.number > 2 else 1.0
        return self.core_tare * mf * tf * ef
    @property
    def fuse_cost(self):
        mf = {Manu.SHORTS: 0.8, Manu.AVRO: 1.05}.get(self.manu, 1.0)
        tf = {FuseType.NORMAL: 1.0,
              FuseType.SLENDER: 1.6,
              FuseType.SLABBY: 0.7,
              FuseType.GEODETIC: 1.1}[self.ftype]
        return self.fuse_tare * 1.2 * mf * tf
    @property
    def wing_cost(self):
        arpen = max(self.wing.ar, 6.0) / 6.0
        ef = 1.2 if self.engines.number > 3 else 1.0
        if self.manu == Manu.SHORTS: # no ef penalty
            return math.pow(self.wing.tare, 1.1) * arpen / 5.0
        mf = {Manu.AVRO: 1.05}.get(self.manu, 1.0)
        return math.pow(self.wing.tare, 1.2) * mf * arpen * ef / 12.0
    @property
    def elec_cost(self):
        if self.elec == Electrics.LOW:
            return 90.0
        if self.elec == Electrics.HIGH:
            return 600.0 / self.engines.number
        assert self.elec == Electrics.STABLE
        base = 500.0 / self.engines.number
        if self.ftype == FuseType.SLENDER:
            return base + 1200.0
        return base + 900.0
    @property
    def cost(self):
        total = self.engines.cost
        total += self.guncost
        total += self.core_cost
        total += self.fuse_cost
        total += self.wing_cost
        return total
    def climb_at_alt(self, alt):
        # step 1: calculate power for level flight
        v = self.wing.minv(self.gross, alt)
        lpwr = self.drag * v / 375.0 # T = D; P = Tv; 1 hp = 375 lbf·mph
        # step 2: surplus power goes into lifting gross weight
        pwr = self.engines.power_at_alt(alt)
        cpwr = (pwr - lpwr) * 0.52 # fudge efficiency of thrust-to-lift conversion
        #print "At", alt, "minv", v, "lpwr", lpwr, "cpwr", cpwr, "maxv", pwr * 375.0 / self.drag
        return cpwr * 33000.0 / self.gross # 1 hp = 33000 lbf·ft/min
    def speed_at_alt(self, alt):
        pwr = self.engines.power_at_alt(alt)
        return pwr * 375.0 / self.drag
    @property
    def takeoff_speed(self):
        return self.wing.minv(self.gross, 0.0) * 1.6 # a certain safety margin is needed
    def all_up_wt(self, v):
        return self.wing.lift(v / 1.6)
    @property
    def range(self):
        return (self.fuel * 0.6 * self.cruising_speed) - 150
    @property
    def ferry(self):
        return self.fuel * self.cruising_speed
    @property
    def ceiling(self):
        #print
        alt = 0.0
        tim = 0.0
        while True:
            c = self.climb_at_alt(alt)
            #print "At", alt, "climb", c, "fpm after", tim, "mins"
            if c < 400 or tim > (45 if self.haddock else 21) or alt >= 35: # above 35,000ft need a pressure cabin (TODO)
                return alt
            alt += 0.5
            tim += 500 / c
    @property
    def cruising_alt(self):
        c = self.ceiling
        return min(c, 10) + max(c - 10, 0) / 2.0
    @property
    def cruising_speed(self):
        return self.speed_at_alt(self.cruising_alt)
    @property
    def fuel_ratio(self):
        return self.fuelmass / self.wing.tare
    @property
    def wing_loading(self):
        return self.gross / self.wing.area
    @property
    def roll_penalty(self):
        return math.sqrt(self.wing.ar)
    @property
    def turn_penalty(self):
        mb = {Manu.BRISTOL: 20.0}.get(self.manu, 18.0)
        return math.sqrt(max(self.wing_loading - mb, 0.0))
    @property
    def manu_penalty(self):
        return self.roll_penalty + self.turn_penalty
    @property
    def evade_factor(self):
        base = max(30.0 - self.ceiling, 3.0) / 10.0
        base *= math.sqrt(max(300.0 - self.cruising_speed, 10.0))
        dodge = 0.3 / max(self.manu_penalty - 4.5, 0.5)
        return base * (1.0 - dodge)
    @property
    def dc(self):
        total = 0.0
        for c in self.crew:
            total += {Crewpos.E: 1.0, Crewpos.W: 0.6}.get(c.pos, 0.0) * (0.75 if c.gun else 1.0)
        return total
    @property
    def vuln(self):
        # engines.  fuse(geodetic).  fuel.  dc.
        base = self.engines.vuln + (0.08 if self.ftype == FuseType.GEODETIC else 0.3 if self.ftype == FuseType.SLENDER else 0.2)
        base *= max(4.0 - self.dc, 1.0)
        base += self.fuel_ratio * (0.5 if self.sst else 1.0) / (5.0 if self.ftype == FuseType.GEODETIC else 4.0)
        return base
    def fight_factor(self, schrage=False):
        return math.pow(self.evade_factor, 0.7) * (self.vuln * 4.0 + self.rate_guns(schrage)) / 3.0
    @property
    def flak_factor(self):
        return self.vuln * 3.0 * math.sqrt(max(30.0 - self.ceiling, 1.0))
    def defn(self, schrage=False):
        return self.fight_factor(schrage) + self.flak_factor

def statblock(b):
    print "Tare:    %5d (wings %5d engines %4d turrets %3d bay %4d fuse %4d tanks %4d)" % (b.tare, b.wing.tare, b.engines.tare, b.guntare, b.bay.tare, b.fuse_tare, b.fuel_tare)
    print "Gross:   %5d (fuel %5d bombs %5d ammo %4d) [AUW %5d/%5d/%5d]" % (b.gross, b.fuelmass, b.bay.cap, b.gunammo, b.all_up_wt(90.0), b.all_up_wt(99.0), b.all_up_wt(108.0))
    print "Drag:    %5d (wings %4d fuse %4d engines %4d turrets %4d; ld %4.1f)" % (b.drag, b.wing_drag, b.fuse_drag, b.engines.drag, b.gundrag, b.wing.ld)
    print "Speed:   %5.1fmph @ SL; %5.1fmph cruise @ %5dft; %5.1fmph take-off" % (b.speed_at_alt(0), b.cruising_speed, 1000 * b.cruising_alt, b.takeoff_speed)
    print "Ceiling: %5dft; init climb %4dfpm.  Svp %d" % (b.ceiling * 1000, b.climb_at_alt(0), b.serv * 100)
    print "Range:   %4dmi (normal); %4dmi (ferry).  Fa %d" % (b.range, b.ferry, b.fail  *100)
    print "Defence: %d (ff %d wl %4.1flb/sqft mp %d ar %3.1f ef %d vu %d gs %d fi %d sc %d fk %d)" % (b.defn(), b.fuel_ratio * 100, b.wing_loading, b.manu_penalty * 100, b.wing.ar, b.evade_factor * 100, b.vuln * 100, b.rate_guns(False) * 100, b.fight_factor(), b.fight_factor(True), b.flak_factor)
    print "Cost:    %5d (core %4d fuse %4d wings %4d engines %4d turrets %4d elec %4d)" % (b.cost, b.core_cost, b.fuse_cost, b.wing_cost, b.engines.cost, b.guncost, b.elec_cost)

mediums = True
whitleys = False
wellies = False
heavies = True
mossies = False
lancs = False
supers = False

if mediums:
    print 'BLEN'
    Blenheim = Bomber(Engines(2, engines[0]), [Chin(), Dorsal()], Wing(469, 56), [Pilot, Nav, Gunner], BombBay(1200, BBGirth.SMALL), 6.4, Manu.BRISTOL, elec=Electrics.LOW)
    statblock(Blenheim) # tare should be 9.8

if whitleys:
    print 'WHT3' # Whitley Mk III
    WhitleyIII = Bomber(Engines(2, engines[11]), [NoseGun(), Ventral(), Tail()], Wing(1137, 84), [Pilot, Nav, Crewman(Crewpos.B, True), Crewman(Crewpos.W, True), Gunner], BombBay(7000, BBGirth.SMALL), 12.0, Manu.ARMSTRONG, ftype=FuseType.SLABBY, elec=Electrics.LOW)
    statblock(WhitleyIII)
    print 'WHIV' # Whitley Mk IV
    WhitleyIV = Bomber(Engines(2, engines[1]), [NoseGun(), TailQuad()], Wing(1137, 84), [Pilot, Nav, Crewman(Crewpos.B, True), WirelessOp, Gunner], BombBay(7000, BBGirth.SMALL), 12.0, Manu.ARMSTRONG, ftype=FuseType.SLABBY, elec=Electrics.LOW)
    statblock(WhitleyIV)
    print 'WHRT' # short-winged Whitley
    Whort = Bomber(Engines(2, engines[2]), [NoseGun(), TailQuad()], Wing(1000, 75), [Pilot, Nav, Crewman(Crewpos.B, True), WirelessOp, Gunner], BombBay(7000, BBGirth.SMALL), 11.0, Manu.ARMSTRONG, ftype=FuseType.SLABBY, elec=Electrics.LOW)
    statblock(Whort)
if mediums or whitleys:
    print 'WHIT'
    Whitley = Bomber(Engines(2, engines[2]), [NoseGun(), TailQuad()], Wing(1137, 84), [Pilot, Nav, Crewman(Crewpos.B, True), WirelessOp, Gunner], BombBay(7000, BBGirth.SMALL), 11.0, Manu.ARMSTRONG, ftype=FuseType.SLABBY)
    statblock(Whitley) # tare should be 19.3
if whitleys:
    print 'ELSW' # speculative Deerhound Whitley
    Elswick = Bomber(Engines(2, engines[10]), [TailQuad()], Wing(1137, 84), [Pilot, Nav, BombAimer, WirelessOp, Gunner], BombBay(7500, BBGirth.SMALL), 11.0, Manu.ARMSTRONG, ftype=FuseType.SLABBY)
    statblock(Elswick)
    print 'ELSH' # short-winged Elswick
    Elshort = Bomber(Engines(2, engines[10]), [TailQuad()], Wing(900, 80), [Pilot, Nav, BombAimer, WirelessOp, Gunner], BombBay(7500, BBGirth.SMALL), 11.0, Manu.ARMSTRONG, ftype=FuseType.SLABBY)
    statblock(Elshort)

if mediums:
    print 'HAMP'
    Hampden = Bomber(Engines(2, engines[5]), [NoseGun(), DorsalAft(), VentralAft()], Wing(668, 69), [Crewman(Crewpos.P, Gunner), Nav, Crewman(Crewpos.W, True), Gunner], BombBay(4000, BBGirth.SMALL), 8, Manu.HP, ftype=FuseType.SLENDER)
    statblock(Hampden) # tare should be 12.75

if mediums or wellies:
    print 'WLNG'
    WellingtonIc = Bomber(Engines(2, engines[5]), [Nose(), Waist(), Tail()], Wing(840, 86), [Pilot, Nav, WirelessOp, Gunner, Gunner, Gunner], BombBay(4500, BBGirth.MEDIUM), 10, Manu.VICKERS, ftype=FuseType.GEODETIC)
    statblock(WellingtonIc) # tare should be 18.5
if wellies:
    print 'WONG' # Long-winged Wellington
    Wellongton = Bomber(Engines(2, engines[5]), [Nose(), Waist(), Tail()], Wing(800, 104), [Pilot, Nav, WirelessOp, Gunner, Gunner, Gunner], BombBay(4500, BBGirth.MEDIUM), 9, Manu.VICKERS, ftype=FuseType.GEODETIC)
    statblock(Wellongton)
    print 'WRUN' # Cleaned-up (unarmed) Wellington
    Wellingrun = Bomber(Engines(2, engines[5]), [], Wing(650, 72), [Pilot, Nav, BombAimer, WirelessOp], BombBay(4500, BBGirth.MEDIUM), 8, Manu.VICKERS, ftype=FuseType.GEODETIC)
    statblock(Wellingrun)
    print 'WLLR' # Wellington with Hampden bombload and no waist guns
    Weller = Bomber(Engines(2, engines[5]), [Nose(), Tail()], Wing(750, 80), [Pilot, Nav, WirelessOp, Gunner, Gunner], BombBay(4000, BBGirth.SMALL), 9, Manu.VICKERS, ftype=FuseType.GEODETIC)
    statblock(Weller)
    print 'WORT' # Short-winged Wellington
    Wort = Bomber(Engines(2, engines[5]), [Nose(), Waist(), Tail()], Wing(700, 75), [Pilot, Nav, WirelessOp, Gunner, Gunner, Gunner], BombBay(4500, BBGirth.MEDIUM), 9.6, Manu.VICKERS, ftype=FuseType.GEODETIC)
    statblock(Wort)
    print 'WIII'
    WellingtonIII = Bomber(Engines(2, engines[8]), [Nose(), TailQuad()], Wing(840, 86), [Pilot, Nav, WirelessOp, Engineer, Gunner, Gunner], BombBay(4500, BBGirth.MEDIUM), 9.4, Manu.VICKERS, ftype=FuseType.GEODETIC)
    statblock(WellingtonIII)
    print 'WMKX'
    WellingtonX = Bomber(Engines(2, engines[9]), [Nose(), TailQuad()], Wing(840, 86), [Pilot, Nav, Crewman(Crewpos.B, True), WirelessOp, Engineer, Gunner], BombBay(4500, BBGirth.MEDIUM), 8.7, Manu.VICKERS, ftype=FuseType.GEODETIC)
    statblock(WellingtonX)
    print 'WONX' # Long-winged Wellington X
    WellongX = Bomber(Engines(2, engines[9]), [Nose(), TailQuad()], Wing(840, 104), [Pilot, Nav, Crewman(Crewpos.B, True), WirelessOp, Engineer, Gunner], BombBay(4500, BBGirth.MEDIUM), 7.8, Manu.VICKERS, ftype=FuseType.GEODETIC)
    statblock(WellongX)
    print 'WSIX' # High-altitude Wellington VI
    Wellingsix = Bomber(Engines(2, engines[4]), [TailQuad()], Wing(890, 98), [Pilot, Nav, BombAimer, Crewman(Crewpos.W, True)], BombBay(4500, BBGirth.MEDIUM), 8, Manu.VICKERS, ftype=FuseType.GEODETIC, haddock=True)
    statblock(Wellingsix)

if mossies:
    print 'MOS4'
    MosquitoIV = Bomber(Engines(2, engines[3]), [], Wing(454, 54), [Pilot, Nav], BombBay(2000, BBGirth.COOKIE), 6.4, Manu.DH, elec=Electrics.STABLE)
    statblock(MosquitoIV)
if mediums or mossies:
    print 'MOSQ'
    MosquitoIX = Bomber(Engines(2, engines[4]), [], Wing(454, 54), [Pilot, Nav], BombBay(4000, BBGirth.COOKIE), 6, Manu.DH, elec=Electrics.STABLE)
    statblock(MosquitoIX) # tare should be 14.3

if heavies:
    print 'MANC'
    Manchester = Bomber(Engines(2, engines[7]), [Nose(), Dorsal(), TailQuad()], Wing(1131, 90), [Pilot, Nav, Crewman(Crewpos.B, True), WirelessOp, Engineer, Gunner, Gunner], BombBay(10350, BBGirth.COOKIE), 10, Manu.AVRO)
    statblock(Manchester) # tare should be 31.2

    print 'STIR'
    Stirling = Bomber(Engines(4, engines[6]), [Nose(), Dorsal(), TailQuad()], Wing(1460, 99), [Pilot, Nav, Crewman(Crewpos.B, True), WirelessOp, Engineer, Gunner, Gunner], BombBay(14000, BBGirth.SMALL), 8, Manu.SHORTS, ftype=FuseType.SLABBY, elec=Electrics.STABLE)
    statblock(Stirling) # tare should be 49.6

if heavies or lancs:
    print 'LANC'
    Lancaster = Bomber(Engines(4, engines[3]), [Nose(), Dorsal(), TailQuad()], Wing(1297, 102), [Pilot, Nav, Crewman(Crewpos.B, True), WirelessOp, Engineer, Gunner, Gunner], BombBay(14000, BBGirth.COOKIE), 9.6, Manu.AVRO, elec=Electrics.STABLE, haddock=True)
    statblock(Lancaster) # tare should be 36.9
if lancs:
    print 'SHNC' # Short-winged Lancaster
    Shankaster = Bomber(Engines(4, engines[3]), [Nose(), Dorsal(), TailQuad()], Wing(1297, 88), [Pilot, Nav, Crewman(Crewpos.B, True), WirelessOp, Engineer, Gunner, Gunner], BombBay(14000, BBGirth.COOKIE), 9.6, Manu.AVRO, elec=Electrics.STABLE, haddock=True)
    statblock(Shankaster)
    print 'LONC' # Long-winged Lancaster
    Longcaster = Bomber(Engines(4, engines[3]), [Nose(), Dorsal(), TailQuad()], Wing(1297, 118), [Pilot, Nav, Crewman(Crewpos.B, True), WirelessOp, Engineer, Gunner, Gunner], BombBay(14000, BBGirth.COOKIE), 9.6, Manu.AVRO, elec=Electrics.STABLE, haddock=True)
    statblock(Longcaster)

if supers:
    print 'SOUT'
    Southampton = Bomber(Engines(4, engines[2]), [Nose(), TailQuad()], Wing(1120, 97), [Pilot, Nav, Crewman(Crewpos.B, True), WirelessOp, Gunner, Gunner], BombBay(16000, BBGirth.SMALL), 7.2, Manu.SUPERMARINE, haddock=True)
    statblock(Southampton)
