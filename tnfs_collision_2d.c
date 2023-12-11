/*
 * tnfs_collision_2d.c
 * 2D road fence collision
 */
#include "tnfs_math.h"
#include "tnfs_base.h"
#include "tnfs_fiziks.h"
#include "tnfs_collision_3d.h"

int crash_pitch = 0;
int DAT_8010d1c4 = 0;
int DAT_80111a40 = 0;

void tnfs_collision_rotate(tnfs_car_data *car_data, int angle, int a3, int fence_side, int road_flag, int fence_angle) {
	int v9;
	int v10;
	int v12;
	int crash_roll;
	signed int crash_yaw;
	signed int collisionAngle;
	signed int rotAngle;
	int v17;

	if (fence_side <= 0)
		v9 = fence_angle + angle;
	else
		v9 = angle - fence_angle;

	rotAngle = v9 - car_data->angle_y + (-(v9 - car_data->angle_y < 0) & 0x1000000);
	if (rotAngle <= 0x800000)
		collisionAngle = v9 - car_data->angle_y + (-(v9 - car_data->angle_y < 0) & 0x1000000);
	else
		collisionAngle = 0x1000000 - rotAngle;
	if (collisionAngle > 0x400000) {
		rotAngle += 0x800000;
		collisionAngle = 0x1000000 - rotAngle;
	}
	if (rotAngle > 0x1000000)
		rotAngle -= 0x1000000;

	if (a3 >= 0x1000000 && car_data->speed > 0x230000) {
		v10 = a3 - 0x60000;
		if (rotAngle >= 0x800000)
			v17 = 0x1000000 - fix3(0x1000000 - rotAngle);
		else
			v17 = fix3(rotAngle);

		if (v17 > 0x800000)
			v17 -= 0x1000000;
		if (v10 > 0xE0000)
			v10 = 0xE0000;

		v12 = fix6(v10 * (collisionAngle >> 16));
		if (v17 <= 0) {
			crash_yaw = -v10;
			crash_roll = -(v10 - v12);
		} else {
			crash_yaw = v10;
			crash_roll = v10 - v12;
		}
		if (road_flag) {
			crash_pitch = -v12;
		} else {
			crash_roll = -crash_roll;
			crash_pitch = fix6(v10 * (collisionAngle >> 16));
		}
		if (crash_yaw != 0) {
			tnfs_collision_rollover_start(car_data->car_data_ptr, fix2(crash_yaw), fix2(3 * crash_roll), fix2(3 * crash_pitch));
		}
	} else {
		if (abs(collisionAngle) < 0x130000 && car_data->speed_local_lon > 0) {
			if (fence_side <= 0) {
				if (rotAngle < 0x130000) {
					car_data->angle_y += rotAngle;
					car_data->angular_speed = 0x9FFFC + 4;
				}
			} else {
				if (rotAngle > 0xed0001) {
					car_data->angle_y += rotAngle;
					car_data->angular_speed -= 0x9FFFC + 4;
				}
			}
		}
	}
}


int tnfs_collision_car_size(tnfs_car_data *car_data, int fence_angle) {
	int x;

	// fast cosine
	x = abs(fence_angle - car_data->angle_y) >> 0x10;
	if (x > 0xc1) {
		x = 0x100 - x;
	} else if (x > 0x80) {
		x = x - 0x80;
	} else if (x > 0x40) {
		x = 0x80 - x;
	}

	return (((car_data->car_length - car_data->car_width) * x) >> 8) + car_data->car_width;
}

void tnfs_track_fence_collision(tnfs_car_data *car_data) {
	int abs_speed;
	int distance;
	int aux;
	long fence_angle;
	int fenceSide;
	int road_flag;
	int rebound_speed_x;
	int rebound_speed_z;
	int rebound_x;
	int rebound_z;
	int local_angle;
	int local_length;
	int pos_x;
	int pos_y;
	int re_speed;
	int lm_speed;

	//fence_angle = (dword_12DECC + 36 * (dword_1328E4 & car_data->road_segment_a) + 22) >> 16 << 10;
	fence_angle = road_segment_heading * 0x400;
	pos_x = math_sin_2(fence_angle >> 8);
	pos_y = math_cos_2(fence_angle >> 8);
	road_flag = 0;
	fenceSide = 0;
	distance = fixmul(pos_x, road_segment_pos_z - car_data->position.z) - fixmul(pos_y, road_segment_pos_x - car_data->position.x);

	rebound_speed_x = 0;
	if (distance < roadLeftMargin * -0x2000) {
		// left road side
		car_data->surface_type_a = roadConstantB >> 4;
		aux = tnfs_collision_car_size(car_data, fence_angle);
		aux = roadLeftFence * -0x2000 + aux;
		if (distance < aux) {
			fenceSide = -0x100;
			rebound_speed_x = aux - distance - 0x8000;
			road_flag = roadConstantA >> 4;
		}

	} else if (distance > roadRightMargin * 0x2000) {
		// right road side
		car_data->surface_type_a = roadConstantB & 0xf;
		aux = tnfs_collision_car_size(car_data, fence_angle);
		aux = roadRightFence * 0x2000 - aux;
		if (distance > aux) {
			fenceSide = 0x100;
			rebound_speed_x = distance + 0x8000 - aux;
			road_flag = roadConstantA & 0xf;
		}
	} else {
		car_data->surface_type_a = 0;
	}
	if (fenceSide == 0) {
		return;
	}

	// impact bounce off
	rebound_speed_x = fixmul(fenceSide, abs(rebound_speed_x));
	rebound_speed_z = 0;

	// reposition the car back off the fence
	math_rotate_2d(rebound_speed_x, 0, fence_angle, &rebound_x, &rebound_z);
	car_data->position.x = car_data->position.x - rebound_x;
	car_data->position.z = car_data->position.z + rebound_z;

	// change speed direction
	math_rotate_2d(-car_data->speed_x, car_data->speed_z, road_segment_heading * -0x400, &rebound_speed_x, &rebound_speed_z);

	abs_speed = abs(rebound_speed_x);
	if ((road_flag == 0) && (abs_speed >= 0x60001)) {
		if (sound_flag == 0) {
			if (car_data->unknown_flag_475 == 0) {
				if (DAT_80111a40 != 0) {
					if (distance <= 0)
						local_angle = 0x280000;
					else
						local_angle = 0xd70000;
					local_length = 1;
				} else {
					tnfs_physics_car_vector(car_data, &local_angle, &local_length);
				}
			}
		} else {
			if (car_data->unknown_flag_475 <= 0) {
				local_angle = 0x400000;
			} else {
				local_angle = 0xc00000;
			}
		}

		//play collision sound
		tnfs_sfx_play(-1, 2, 9, abs_speed, local_length, local_angle);
		if (abs_speed > 0x140000) {
			tnfs_debug_000502AB(0x32);
		}
	}

	// limit collision speed
	if (abs_speed > 0x180000) {
		if (rebound_speed_x > +0x20000)
			rebound_speed_x = +0x20000;
		if (rebound_speed_x < -0x20000)
			rebound_speed_x = -0x20000;
	} else {
		rebound_speed_x = fix2(rebound_speed_x);
	}
	if (rebound_speed_z < -0x30000) {
		if ((DAT_8010d1c4 & 0x20) == 0) {
			rebound_speed_z = abs_speed + rebound_speed_z;
			lm_speed = fix2(abs_speed);
		} else {
			lm_speed = abs_speed / 2;
		}
		rebound_speed_z = rebound_speed_z + lm_speed;
		if (0 < rebound_speed_z) {
			rebound_speed_z = 0;
		}
	} else if (rebound_speed_z > 0x30000) {
		if ((DAT_8010d1c4 & 0x20) == 0) {
			lm_speed = fix2(abs_speed);
			aux = abs_speed;
		} else {
			lm_speed = fix3(abs_speed);
			aux = abs_speed / 2;
		}
		rebound_speed_z = rebound_speed_z - lm_speed - aux;
		if (rebound_speed_z < 0) {
			rebound_speed_z = 0;
		}
	}

	// set collision angle side
	re_speed = fix2(abs_speed);
	if (re_speed > 0xa0000) {
		re_speed = 0xa0000;
	}
	if (fenceSide < 1) {
		fence_angle = fence_angle + 0x20000; //+180
		car_data->collision_x = -re_speed;
	} else {
		fence_angle = fence_angle - 0x20000; //-180
		car_data->collision_x = re_speed;
	}

	car_data->collision_y = -re_speed;
	car_data->collision_a = 0x1e;
	car_data->collision_b = 0x1e;

	// reflect car speeds 180deg
	//FIXME (fence_angle * 0x400) for correct angle scale
	math_rotate_2d(rebound_speed_x, rebound_speed_z, fence_angle * 0x400, &car_data->speed_x, &car_data->speed_z);

	// ???
	if (road_flag == 0) {
		local_angle = 0x30000;
		road_flag = 0;
	} else {
		local_angle = 0x18000;
	}
	tnfs_collision_rotate(car_data, fence_angle, abs_speed, fenceSide, road_flag, local_angle);
	if (car_data->speed_y > 0) {
		car_data->speed_y = 0;
	}
}

