# Meteor steering

Compared with the original keyboard steering, the wheels turn faster while a steering key is held (near the centre
1.2 times as fast at low speed, up to twice as fast from 122 km/h, so that turning stays at or just below a steering
wheel or pad's on every car), and when the keys are released they return to the centre slower than the original's 8 times the
steering speed: at small angles twice the steering speed at high speed (4 times slower), 4 times below it (twice as
slow), going up to the original's return at full lock. The pedal keys cut the throttle 3 times as fast when you lift
off or brake, as fast as a pad lifted off its pedal, and open it 3 times as fast, as fast as a pad's full pedal.

A **port extra** (no TNFS binary has it), designed by a player so that keyboard players can keep up with analog
steering. It is **off by default**, and with it off the port behaves exactly as before.

## Model

Only the rate at which `tnfs_control_steering_a` (keyboard steering) moves `steer_angle` changes, once per control
step (15 per second):

- **Steering out** (a steering key held): a factor at the centre that goes by the speed, going down linearly to x1 at
  75% of full lock, x1 beyond. The factor: x1.2 up to 54 km/h, x1.35 at 72, x1.75 at 90, x1.8 at 108, x2 from 122 km/h,
  linear between (`g_meteor_out_rate`).
- **Every car alike**: the rate comes from the PBS steering ramp shared by 8 of the 9 cars (`TNFS_METEOR_VEL_RAMP`);
  the Corvette's higher ramp (quicker keyboard steering from 54 km/h up) is not used, as a wheel or pad steers every
  car the same.
- **Return** (no steering key held): a multiple of the rate at which the original steers out at the same speed. The
  original returns 8 times as fast as it steers out while moving forward (PBS ramp shifts, the same for every car), at
  standstill and in reverse at the steering out rate. The multiple goes by the angle the wheels are at and the speed:
  - up to 25% of full lock: x4 up to 50 m/s (180 km/h), going linearly down to x2 at 67.5 m/s (243 km/h), x2
    beyond: half the original's speed, then a quarter, for holding gentle high speed curves by tapping;
  - from 25% of full lock: going linearly up to x8 at full lock, the original's while moving forward, so a release
    from a large angle starts at the original's speed and slows down near the centre. x16 (faster corner exits) was
    tried and dropped: a let-go of 2 control steps between long taps then centred the wheels, which broke tapping
    through a curve.

  At standstill and in reverse that is 4 to 8 times the original's return (a gamepad's stick returns the wheels at
  once anyway).

The factors apply after the original rate (speed attenuation, caps, PBS ramp shifts) and before the clamp to the
distance left, so the angle never overshoots. Integer math, no state: the rate goes by the control step's keys, speed and
steering angle only.

## Pedal keys

In `tnfs_control_pedals_digital`, with the throttle key released or the brake key held, the throttle ramps down 3
times per control step (`TNFS_METEOR_THROTTLE_OFF_RATE`): 51 per step instead of the PBS off ramp's 17 (the same on
every car), so full throttle is cut in 5 steps (0.33 s) instead of 15 (1 s). That is how fast a pad lifted off its
pedal cuts it (`tnfs_control_pedals_analog`, 0x33 per step in gear); a pad that goes straight to the brake cuts it at
once. With the throttle key held it ramps up 3 times per control step (`TNFS_METEOR_THROTTLE_ON_RATE`): 75 per step
instead of the PBS on ramp's 25 (every car), full throttle in 4 steps (0.27 s) instead of 10, as a pad's full pedal
(0x4c per step). The brake ramp is unchanged: building the brake up faster brakes less.

The whole braking gap between the key and a full pad pedal is the throttle the key keeps while braking: cutting it at
once matches the pad exactly, on cars with and without ABS. Distances to slow down (Diablo / Supra, straight road):

| | key | meteor | pad, full pedal |
|---|---|---|---|
| braking 200 -> 150 km/h | 81.7 / 84.3 m | 79.7 / 79.2 m | 78.0 / 78.8 m |
| braking 108 -> 54 km/h | 47.7 / 46.3 m | 43.7 / 43.8 m | 42.4 / 41.8 m |
| lifting off 144 -> 130 km/h | 88.7 / 78.6 m | 72.7 / 62.7 m | 72.7 / 62.7 m |

Braking closes 51 to 94% of the gap to a pad (x2 measured 11 to 64%, x5 59 to 97%); lifting off matches a pad.

Accelerating after a lift-off (throttle 0, in gear), seconds to reach the speed, key / meteor / pad: Diablo 90 -> 126
km/h 4.00 / 3.40 / 3.40, 162 -> 198 km/h 5.47 / 5.00 / 5.00; Supra 54 -> 108 km/h 4.30 / 3.80 / 3.90; P911 54 -> 108
km/h 5.60 / 4.30 / 4.30. Meteor is within 0.1 s of a pad (the key was 0.3 to 1.3 s behind; x2 measured 67 to 92% of
the gap).

## Balance against a wheel

How far the car turns in 1 s of full steering held (a full-lock corner, from a straight, throttle held), as a share of
a wheel or pad at full lock, all 9 cars: 97 to 101% from 36 to 117 km/h, 93 to 98% from 126 km/h. Without the speed
factor (x2 everywhere) meteor was ahead of a wheel below 100 km/h (104 to 109% at 54 to 72 km/h) and the Corvette up to
117 km/h. Below 36 km/h the test is not reliable (cars spin). The factors are the largest that keep every car at or just
under a wheel: the boost each car needs to match a wheel was measured per speed, the lowest taken.

## Effect

Control steps with the keys alone at a fixed physics speed (`speed_local_lon`), original -> meteor, auto steer off as on
PC. With meteor every car steers the same; the original column is the other 8 cars (the Corvette's ramp is higher):

| speed | centre to full lock | full lock to centre | 50% of lock to centre | 2-step tap reaches | 2-step tap, steering x time |
|---|---|---|---|---|---|
| 0 m/s | 5 -> 4 | 5 -> 1 | 3 -> 1 | 47% -> 55% of lock | 0.95 -> 0.84 |
| 3 to 15 m/s | 9 -> 8 | 2 -> 2 | 1 -> 1 | 24% -> 28% | 0.36 -> 0.42 |
| 20 m/s | 10 -> 9 | 2 -> 2 | 1 -> 1 | 21% -> 27% | 0.31 -> 0.41 |
| 25 m/s | 12 -> 9 | 2 -> 2 | 1 -> 2 | 18% -> 30% | 0.27 -> 0.45 |
| 27.8 m/s | 13 -> 10 | 2 -> 2 | 1 -> 2 | 17% -> 28% | 0.25 -> 0.43 |
| 30 m/s | 14 -> 11 | 2 -> 3 | 1 -> 2 | 15% -> 26% | 0.22 -> 0.39 |
| 34 m/s | 17 -> 13 | 3 -> 3 | 2 -> 2 | 12% -> 24% | 0.19 -> 0.36 |
| 37.5 to 50 m/s | 19..20 -> 15 | 3 -> 4 | 2 -> 3 | 10% -> 20% | 0.16 -> 0.30 |
| 55.6 m/s (200 km/h) | 20 -> 15 | 3 -> 4 | 2 -> 3 | 10% -> 20% | 0.16 -> 0.33 |
| 62.5 m/s | 20 -> 15 | 3 -> 5 | 2 -> 3 | 10% -> 20% | 0.16 -> 0.37 |
| 65 m/s | 20 -> 15 | 3 -> 5 | 2 -> 4 | 10% -> 20% | 0.16 -> 0.39 |
| 67.5 m/s and up | 20 -> 15 | 3 -> 5 | 2 -> 4 | 10% -> 20% | 0.16 -> 0.40 |

Steering x time: the angle after each step of the tap, summed until the wheels are centred, in full lock x control
steps (how much the tap steers). While moving, a 2-step tap is centred 1 step after the release in the original and
with meteor up to 50 m/s, 2 steps after it with meteor above 50 m/s. A release from small angles (20% of lock) moves
20% of lock per step up to 50 m/s, 17.4% at 55.6 m/s (200 km/h), 13.3% at 62.5 m/s and 10.4% from 67.5 m/s.

Full lock to centre at 67.5 m/s and up, % of lock after each control step: original 100, 58.5, 17, 0; meteor 100,
58.5, 34.2, 20, 9.7, 0.

## Using it

- Engine: set `g_meteor_steering = 1` before `tnfs_init_sim`. Play a replay with the setting it was recorded with: the
  `.RPL` format (SE's) has no field for it.
- SDL front end: **M** toggles it, before the race.
- nfs-web's wrapper (not the engine) also keeps a steering key press until the next control step has read it while
  meteor steering is on, so a tap shorter than a control step (66.7 ms) always steers for one step. The original
  reads the keys' state once per step, and such a tap counts only if a read falls inside it.
- Tuning: in `tnfs_base.c`: `g_meteor_out_rate` (steering out factor at the centre by speed, 16.16 points),
  `TNFS_METEOR_VEL_RAMP` 0x23333 (the common steering ramp), `TNFS_METEOR_OUT_END` 0x144000 (75% of lock, back to
  x1); the return's factors on the steering out rate (16.16):
  `TNFS_METEOR_RETURN_SLOW_RATE` 0x40000 (x4, small angles up to `TNFS_METEOR_RETURN_SLOW_SPEED` 0x320000, 50 m/s),
  `TNFS_METEOR_RETURN_RATE` 0x20000 (x2, small angles from `TNFS_METEOR_RETURN_FAST_SPEED` 0x438000, 67.5 m/s),
  `TNFS_METEOR_RETURN_ANGLE` 0x6c000 (25% of lock, the end of the small angles), `TNFS_METEOR_RETURN_LOCK_RATE`
  0x80000 (x8 at full lock).

## Verification

AL1 with the Supra, cop and traffic, 3000 ticks of scripted keyboard driving:
- Off: every car struct on every tick and the `.RPL` are the same as before the change.
- On: a playback from clock 0 in a fresh process matches the recorded race on every tick, and so do seeks to 6 clocks
  (position, angles, steering, speed, slice, lap and crash state of every car).

The return by angle and speed (2026-11-17): the effect table measured by calling `tnfs_control_steering_a` with
LDIABL's PBS at fixed speeds; meteor off runs none of the changed code.
