/*
 * globals, structs, and stub TNFS functions
 */
#include "tnfs_math.h"
#include "tnfs_base.h"
#include "tnfs_files.h"
#include "tnfs_fiziks.h"
#include "tnfs_collision_3d.h"

tnfs_car_specs car_specs;
tnfs_car_data car_data;
tnfs_track_data track_data[2400];
tnfs_surface_type road_surface_type_array[3] = { { 0x100, 0x100, 0x3333, 0 }, { 0x100, 0x1400, 0x3333, 1}, { 0x100, 0x1400, 0x3333, 1} };

// settings/flags
char is_drifting;
int g_game_time = 1000;
char roadConstantA = 0x20;
char roadConstantB = 0x22;
int road_segment_count = 0;
int sound_flag = 0;
int cheat_crashing_cars = 0;
int cheat_code_8010d1c4 = 0;

int selected_camera = 0;
tnfs_vec3 camera_position;

/* tire grip slide table  */
static const unsigned char g_slide_table[64] = {
		// front values
		2, 14, 26, 37, 51, 65, 101, 111, 120, 126, 140, 150, 160, 165, 174, 176, 176, 176, 176, 176, 176, 177, 177, 177, 200, 200, 200, 174, 167, 161, 152, 142,
		// rear values
		174, 174, 174, 174, 175, 176, 177, 177, 200, 200, 200, 200, 177, 177, 177, 177, 177, 177, 177, 177, 177, 177, 177, 177, 200, 176, 173, 170, 165, 162, 156, 153
};

/* engine torque table, for 200 rpm values */
static const unsigned int g_torque_table[120] = {
		1000, 440, 1200, 450, 1400, 460, 1600, 470, 1800, 480, 2000, 490, 2200, 530, 2400, 560, 2600, 570, 2800, 580,
		3000, 590, 3200, 592, 3400, 600, 3600, 610, 3800, 600, 4000, 580, 4200, 570, 4400, 570, 4600, 560, 4800, 560,
		5000, 560, 5200, 550, 5400, 550, 5600, 530, 5800, 510, 6000, 500, 6200, 490, 6400, 480, 6600, 470, 6800, 460
};


void auto_generate_track() {
	int pos_x = 0;
	int pos_y = 0;
	int pos_z = 0;
	int slope = 0;
	int slant = 0;
	int rnd = 0;

	road_segment_count = 2400;

	for (int i = 0; i < 2400; i++) {

		if (i % 30 == 0)
			rnd = rand();

		if (rnd & 128) {
			if (rnd & 64) {
				slope -= 10;
			} else {
				slope += 10;
			}
		} else {
			slope *= 0.9;
		}
		if (rnd & 32) {
			if (rnd & 16) {
				slant -= 15;
			} else {
				slant += 15;
			}
		} else {
			slant *= 0.9;
		}

		if (slope > 0x3FF) slope = 0x3FF;
		if (slope < -0x3FF) slope = -0x3FF;
		if (slant > 0x3FF) slant = 0x3FF;
		if (slant < -0x3FF) slant = -0x3FF;

		track_data[i].roadLeftFence = 0x50;
		track_data[i].roadRightFence = 0x50;
		track_data[i].roadLeftMargin = 0x35;
		track_data[i].roadRightMargin = 0x35;

		track_data[i].slope = slope;
		track_data[i].heading = -slant << 2;
		track_data[i].slant = slant;
		track_data[i].pos.x = pos_x;
		track_data[i].pos.y = pos_y;
		track_data[i].pos.z = pos_z;

		// next segment
		pos_x += fixmul(math_sin_3(track_data[i].heading * 0x400), 0x80000);
		pos_y += fixmul(math_tan_3(track_data[i].slope * 0x400), 0x80000);
		pos_z += fixmul(math_cos_3(track_data[i].heading * 0x400), 0x80000);
	}
}

void tnfs_init_track(char * tri_file) {
	int i;
	int heading, s, c, t, dL, dR;

	// try to read a TRI file if given, if not, generate a random track
	if (!read_tri_file(tri_file)) {
		auto_generate_track();
	}

	// model track for rendering
	for (i = 0; i < road_segment_count; i++) {
		heading = track_data[i].heading * -0x400;
		s = math_sin_3(heading);
		c = math_cos_3(heading);
		t = math_tan_3(track_data[i].slant * 0x400);

		dL = -track_data[i].roadLeftMargin * 0x2000;
		dR = track_data[i].roadRightMargin * 0x2000;

		track_data[i].side_point.x = fixmul(c, dR);
		track_data[i].side_point.y = fixmul(t, dR);
		track_data[i].side_point.z = fixmul(s, dR);

		track_data[i].wall_normal.x = c;
		track_data[i].wall_normal.y = fixmul(t, 0x10000);
		track_data[i].wall_normal.z = s;

		track_data[i].vf_margin_L.x = (float) (track_data[i].pos.x + fixmul(c, dL)) / 0x10000;
		track_data[i].vf_margin_L.y = (float) (track_data[i].pos.y + fixmul(t, dL)) / 0x10000;
		track_data[i].vf_margin_L.z = -(float) (track_data[i].pos.z + fixmul(s, dL)) / 0x10000;
		track_data[i].vf_margin_R.x = (float) (track_data[i].pos.x + fixmul(c, dR)) / 0x10000;
		track_data[i].vf_margin_R.y = (float) (track_data[i].pos.y + fixmul(t, dR)) / 0x10000;
		track_data[i].vf_margin_R.z = -(float) (track_data[i].pos.z + fixmul(s, dR)) / 0x10000;

		dL = -track_data[i].roadLeftFence * 0x2000;
		dR = track_data[i].roadRightFence * 0x2000;

		track_data[i].vf_fence_L.x = (float) (track_data[i].pos.x + fixmul(c, dL)) / 0x10000;
		track_data[i].vf_fence_L.y = (float) (track_data[i].pos.y + fixmul(t, dL)) / 0x10000;
		track_data[i].vf_fence_L.z = -(float) (track_data[i].pos.z + fixmul(s, dL)) / 0x10000;
		track_data[i].vf_fence_R.x = (float) (track_data[i].pos.x + fixmul(c, dR)) / 0x10000;
		track_data[i].vf_fence_R.y = (float) (track_data[i].pos.y + fixmul(t, dR)) / 0x10000;
		track_data[i].vf_fence_R.z = -(float) (track_data[i].pos.z + fixmul(s, dR)) / 0x10000;
	}
}

void tnfs_create_car_specs() {
	int i;

	car_specs.mass_front = 0x3148000; //788kg
	car_specs.mass_rear = 0x3148000; //788kg
	car_specs.mass_total = 0x6290000; //1577kg
	car_specs.inverse_mass = 0x29; // 1/1577
	car_specs.front_drive_percentage = 0; //RWD
	car_specs.front_brake_percentage = 0xc000; //70%
	car_specs.centre_of_gravity_height = 0x7581; //0.459
	car_specs.max_brake_force_1 = 0x133fff;
	car_specs.max_brake_force_2 = 0x133fff;
	car_specs.drag = 0x8000; //0.5
	car_specs.top_speed = 0x47cccc; //71m/s
	car_specs.efficiency = 0xb333; //0.7
	car_specs.wheelbase = 0x270A3; //2.44m
	car_specs.burnOutDiv = 0x68EB; //0.4
	car_specs.wheeltrack = 0x18000; //1.50m
	car_specs.mps_to_rpm_factor = 0x59947a; //conversion=rpm/speed/gear=1200/26.79/0.5= 26.79
	car_specs.number_of_gears = 8;
	car_specs.final_drive = 0x311eb; //3.07
	car_specs.wheel_roll_radius = 0x47ae; //28cm
	car_specs.inverse_wheel_radius = 0x3924a; //3.57
	car_specs.gear_ratio_table[0] = -152698; //-2.33 reverse
	car_specs.gear_ratio_table[1] = 0x1999; //neutral
	car_specs.gear_ratio_table[2] = 0x2a8f5; //2.66
	car_specs.gear_ratio_table[3] = 0x1c7ae; //1.78
	car_specs.gear_ratio_table[4] = 0x14ccc; //1.30
	car_specs.gear_ratio_table[5] = 0x10000; //1.00
	car_specs.gear_ratio_table[6] = 0xcccc;  //0.75
	car_specs.gear_ratio_table[7] = 0x8000;  //0.50
	car_specs.torque_table_entries = 0x33; //60;
	car_specs.front_roll_stiffness = 0x2710000; //10000
	car_specs.rear_roll_stiffness = 0x2710000; //10000
	car_specs.roll_axis_height = 0x476C; //0.279
	car_specs.cutoff_slip_angle = 0x1fe667; //~45deg
	car_specs.rpm_redline = 6000;
	car_specs.rpm_idle = 500;

	// torque table
	memcpy(car_specs.torque_table, &g_torque_table, sizeof(g_torque_table));

	car_specs.gear_upshift_rpm[0] = 5900;
	car_specs.gear_upshift_rpm[1] = 5900;
	car_specs.gear_upshift_rpm[2] = 5900;
	car_specs.gear_upshift_rpm[3] = 5900;
	car_specs.gear_upshift_rpm[4] = 5900;
	car_specs.gear_upshift_rpm[5] = 5900;
	car_specs.gear_upshift_rpm[6] = 5900;

	car_specs.gear_efficiency[0] = 0x100;
	car_specs.gear_efficiency[1] = 0x100;
	car_specs.gear_efficiency[2] = 0x100;
	car_specs.gear_efficiency[3] = 0xdc;
	car_specs.gear_efficiency[4] = 0x10e;
	car_specs.gear_efficiency[5] = 0x113;
	car_specs.gear_efficiency[6] = 0x113;
	car_specs.gear_efficiency[7] = 0x113;

	car_specs.inertia_factor = 0x8000; //0.5
	car_specs.body_roll_factor = 0x2666; //0.15
	car_specs.body_pitch_factor = 0x2666; //0.15
	car_specs.front_friction_factor = 0x2b331;
	car_specs.rear_friction_factor = 0x2f332;
	car_specs.body_length = 0x47333; //4.45m
	car_specs.body_width = 0x1eb85; //1.92m
	car_specs.lateral_accel_cutoff = 0x158000;
	car_specs.final_drive_torque_ratio = 0x280;
	car_specs.thrust_to_acc_factor = 0x66;
	car_specs.shift_timer = 3;
	car_specs.noGasRpmDec = 0x12c; //300
	car_specs.garRpmInc = 0x258; //600
	car_specs.clutchDropRpmDec = 0xb4; //180
	car_specs.clutchDropRpmInc = 0x12c; //300
	car_specs.negTorque = 0xd; //0.0001
	car_specs.ride_height = 0x1c0cc; //1.05
	car_specs.centre_y = 0x4c; //76

	// tire grip-slip angle tables
	for (i = 0; i < 1024; i++) {
		car_specs.grip_table[i] = g_slide_table[i >> 4];
	}
}

void tnfs_reset_car() {
	int aux;

	car_data.car_length = car_specs.body_length;
	car_data.car_width = car_specs.body_width;

	car_data.weight_distribution_front = fixmul(car_specs.mass_front, car_specs.inverse_mass);
	car_data.weight_distribution_rear = fixmul(car_specs.mass_rear, car_specs.inverse_mass);
	car_data.weight_transfer_factor = fixmul(car_specs.centre_of_gravity_height, car_specs.burnOutDiv);

	aux = fixmul(fixmul(car_specs.wheelbase, car_specs.wheelbase), 0x324);

	car_data.wheel_base = math_div(aux, car_specs.wheelbase);
	car_data.front_yaw_factor = math_div(fixmul(car_specs.wheelbase, car_data.weight_distribution_front), aux);
	car_data.rear_yaw_factor = math_div(fixmul(car_specs.wheelbase, car_data.weight_distribution_rear), aux);

	car_data.front_friction_factor = fixmul(0x9cf5c, fixmul(car_specs.front_friction_factor, car_data.weight_distribution_rear));
	car_data.rear_friction_factor = fixmul(0x9cf5c, fixmul(car_specs.rear_friction_factor, car_data.weight_distribution_front));

	car_data.tire_grip_front = car_data.front_friction_factor;
	car_data.tire_grip_rear = car_data.rear_friction_factor;

	car_data.collision_height_offset = 0x92f1;
	car_data.collision_data.linear_acc_factor = 0xf646;
	car_data.collision_data.angular_acc_factor = 0x7dd4;
	car_data.collision_data.size.x = car_specs.body_width / 2;
	car_data.collision_data.size.y = 0x92f1;
	car_data.collision_data.size.z = car_specs.body_length / 2;

	car_data.car_data_ptr = &car_data;
	car_data.car_specs_ptr = &car_specs;

	car_data.gear_selected = -1; //-2 Reverse, -1 Neutral, 0..8 Forward gears
	car_data.gear_auto_selected = 2; //0 Manual mode, 1 Reverse, 2 Neutral, 3 Drive
	car_data.gear_shift_current = -1;
	car_data.gear_shift_previous = -1;
	car_data.gear_shift_interval = 16;
	car_data.tire_skid_front = 0;
	car_data.tire_skid_rear = 0;
	car_data.is_gear_engaged = 0;
	car_data.handbrake = 0;
	car_data.is_engine_cutoff = 0;
	car_data.is_shifting_gears = -1;
	car_data.throttle_previous_pos = 0;
	car_data.throttle = 0;
	car_data.tcs_on = 0;
	car_data.tcs_enabled = 0;
	car_data.brake = 0;
	car_data.abs_on = 0;
	car_data.abs_enabled = 0;
	car_data.is_crashed = 0;
	car_data.is_wrecked = 0;
	car_data.time_off_ground = 0;
	car_data.slide_front = 0;
	car_data.slide_rear = 0;
	car_data.wheels_on_ground = 1;
	car_data.surface_type = 0;
	car_data.surface_type_b = 0;
	//car_data.road_segment_a = 0;
	//car_data.road_segment_b = 0;
	car_data.slope_force_lat = 0;
	car_data.unknown_flag_3DD = 0;
	car_data.slope_force_lon = 0;
	car_data.position.x = track_data[car_data.road_segment_a].pos.x;
	car_data.position.y = track_data[car_data.road_segment_a].pos.y + 150;
	car_data.position.z = track_data[car_data.road_segment_a].pos.z;
	car_data.angle_x = track_data[car_data.road_segment_a].slope * 0x400;
	car_data.angle_y = track_data[car_data.road_segment_a].heading * 0x400;
	car_data.angle_z = track_data[car_data.road_segment_a].slant * 0x400;
	car_data.body_pitch = 0;
	car_data.body_roll = 0;
	car_data.angle_dx = 0;
	car_data.angular_speed = 0;
	car_data.speed_x = 0;
	car_data.speed_y = 0;
	car_data.speed_z = 0;
	car_data.speed = 0;
	car_data.speed_drivetrain = 0;
	car_data.speed_local_lat = 0;
	car_data.speed_local_vert = 0;
	car_data.speed_local_lon = 0;
	car_data.steer_angle = 0; //int32 -1769472 to +1769472
	car_data.tire_grip_loss = 0;
	car_data.susp_incl_lat = 0;
	car_data.susp_incl_lon = 0;
	car_data.road_grip_increment = 0;
	car_data.lap_number = 1;
	car_data.field203_0x174 = 0x1e0;
	car_data.field444_0x520 = 0;
	car_data.field445_0x524 = 0;
	car_data.unknown_flag_475 = 0;
	car_data.unknown_flag_479 = 0;
	car_data.unknown_flag_480 = 0;
	car_data.world_position.x = 0;
	car_data.world_position.y = 0;
	car_data.world_position.z = 0;
	car_data.road_ground_position.x = 0;
	car_data.road_ground_position.y = 0;
	car_data.road_ground_position.z = 0;

	car_data.rpm_vehicle = car_specs.rpm_idle;
	car_data.rpm_engine = car_specs.rpm_idle;
	car_data.rpm_redline = car_specs.rpm_redline;

	car_data.road_fence_normal.x = 0x10000;
	car_data.road_fence_normal.y = 0;
	car_data.road_fence_normal.z = 0;

	//surface normal (up)
	car_data.road_surface_normal.x = 0;
	car_data.road_surface_normal.y = 0x10000;
	car_data.road_surface_normal.z = 0;

	//track next node (north)
	car_data.road_heading.x = 0;
	car_data.road_heading.y = 0;
	car_data.road_heading.z = 0x10000;

	//surface position center
	car_data.road_position.x = 0;
	car_data.road_position.y = 0;
	car_data.road_position.z = 0;

	car_data.front_edge.x = 0x10000;
	car_data.front_edge.y = 0;
	car_data.front_edge.z = 0;

	car_data.side_edge.x = 0;
	car_data.side_edge.y = 0x10000;
	car_data.side_edge.z = 0;

	math_matrix_identity(&car_data.matrix);
	math_matrix_identity(&car_data.collision_data.matrix);

	car_data.collision_data.position.x = 0;
	car_data.collision_data.position.y = 0;
	car_data.collision_data.position.z = 0;
	car_data.collision_data.speed.x = 0;
	car_data.collision_data.speed.y = 0;
	car_data.collision_data.speed.z = 0;
	car_data.collision_data.field4_0x48.x = 0;
	car_data.collision_data.field4_0x48.y = 0;
	car_data.collision_data.field4_0x48.z = 0;
	car_data.collision_data.crashed_time = 0;
	car_data.collision_data.angular_speed.x = 0;
	car_data.collision_data.angular_speed.y = 0;
	car_data.collision_data.angular_speed.z = 0;
	car_data.collision_data.field6_0x60 = 0;
}

void tnfs_init_car() {
	int i;

	// load car specs
	if (!read_pbs_file("carspecs.pbs")) {
		tnfs_create_car_specs();
	}

	// correct drag coefficient
	car_specs.drag = fixmul(car_specs.drag, car_specs.inverse_mass);

	// correct torque values
	i = 1;
	do {
		car_specs.torque_table[i] = //
				math_mul(math_mul(math_mul(math_mul(
						car_specs.torque_table[i] << 0x10,
						car_specs.final_drive),
						car_specs.efficiency),
						car_specs.inverse_wheel_radius),
						car_specs.inverse_mass);
		i += 2;
	} while (i < car_specs.torque_table_entries);

	tnfs_reset_car();
}

void tnfs_change_camera() {
	selected_camera++;
	if (selected_camera > 1)
		selected_camera = 0;
}

void tnfs_change_gear_automatic(int shift) {
	car_data.gear_auto_selected += shift;

	switch (car_data.gear_auto_selected) {
	case 1:
		car_data.gear_selected = -2;
		car_data.is_gear_engaged = 1;
		printf("Gear: Reverse\n");
		break;
	case 2:
		car_data.gear_selected = -1;
		car_data.is_gear_engaged = 0;
		printf("Gear: Neutral\n");
		break;
	case 3:
		car_data.gear_selected = 0;
		car_data.is_gear_engaged = 1;
		printf("Gear: Drive\n");
		break;
	}
}

void tnfs_change_gear_manual(int shift) {
	car_data.gear_selected += shift;

	switch (car_data.gear_selected) {
	case -2:
		car_data.is_gear_engaged = 1;
		printf("Gear: Reverse\n");
		break;
	case -1:
		car_data.is_gear_engaged = 0;
		printf("Gear: Neutral\n");
		break;
	default:
		car_data.is_gear_engaged = 1;
		printf("Gear: %d\n", car_data.gear_selected + 1);
		break;
	}
}

void tnfs_change_gear_up() {
	if (car_data.gear_auto_selected == 0) {
		if (car_data.gear_selected < car_specs.number_of_gears - 1)
			tnfs_change_gear_manual(+1);
	} else {
		if (car_data.gear_auto_selected < 3)
			tnfs_change_gear_automatic(+1);
	}
}

void tnfs_change_gear_down() {
	if (car_data.gear_auto_selected == 0) {
		if (car_data.gear_selected > -2)
			tnfs_change_gear_manual(-1);
	} else {
		if (car_data.gear_auto_selected > 1)
			tnfs_change_gear_automatic(-1);
	}
}

void tnfs_abs() {
	if (car_data.abs_enabled) {
		car_data.abs_enabled = 0;
		printf("ABS brakes off\n");
	} else {
		car_data.abs_enabled = 1;
		printf("ABS brakes on\n");
	}
}

void tnfs_tcs() {
	if (car_data.tcs_enabled) {
		car_data.tcs_enabled = 0;
		printf("Traction control off\n");
	} else {
		car_data.tcs_enabled = 1;
		printf("Traction control on\n");
	}
}

void tnfs_change_transmission_type() {
	if (car_data.gear_auto_selected == 0) {
		printf("Automatic Transmission mode\n");
		car_data.gear_auto_selected = 2;
		tnfs_change_gear_automatic(0);
	} else {
		printf("Manual Transmission mode\n");
		car_data.gear_auto_selected = 0;
		tnfs_change_gear_manual(0);
	}
}

void tnfs_change_traction() {
	if (car_data.car_specs_ptr->front_drive_percentage == 0x8000) {
		car_data.car_specs_ptr->front_drive_percentage = 0x10000;
		printf("Traction: FWD\n");
	} else if (car_data.car_specs_ptr->front_drive_percentage == 0) {
		car_data.car_specs_ptr->front_drive_percentage = 0x8000;
		printf("Traction: AWD\n");
	} else {
		car_data.car_specs_ptr->front_drive_percentage = 0;
		printf("Traction: RWD\n");
	}
}

void tnfs_cheat_mode() {
	if (cheat_crashing_cars == 4) {
		cheat_crashing_cars = 0;
		printf("Crashing cars cheat enabled - Press handbrake to crash\n");
	} else {
		cheat_crashing_cars = 4;
		printf("Crashing cars cheat disabled\n");
	}
}

void tnfs_crash_car() {
	tnfs_collision_rollover_start(&car_data, 0, 0, -0xa0000);
}

void tnfs_sfx_play(int a, int b, int c, int d, int e, int f) {
	printf("sound %i\n", f);
}

void tnfs_replay_highlight_000502AB(char a) {
	printf("replay highlight %i\n", a);
}

/* common TNFS functions */

void tnfs_car_local_position_vector(tnfs_car_data *car_data, int *angle, int *length) {
	int x;
	int y;
	int z;
	int heading;

	x = car_data->position.x - track_data[car_data->road_segment_a].pos.x;
	y = car_data->position.y - track_data[car_data->road_segment_a].pos.y;
	z = car_data->position.z - track_data[car_data->road_segment_a].pos.z;

	heading = track_data[car_data->road_segment_a].heading * 0x400;

	if (heading < 0) {
		heading = heading + 0x1000000;
	}
	*angle = heading - math_atan2(z, x);
	if (*angle < 0) {
		*angle += 0x1000000;
	}
	if (*angle > 0x1000000) {
		*angle -= 0x1000000;
	}

	if (x < 0) {
		x = -x;
	}
	if (y < 0) {
		y = -y;
	}
	if (z < 0) {
		z = -z;
	}
	if (z < x) {
		x = (z >> 2) + x;
	} else {
		x = (x >> 2) + z;
	}
	if (x < y) {
		*length = (x >> 2) + y;
	} else {
		*length = (y >> 2) + x;
	}
}

int tnfs_road_segment_find(tnfs_car_data *car_data, int *current) {
	int node;
	int dist1;
	int dist2;
	int changed;
	tnfs_vec3 position;
	struct tnfs_track_data *tracknode1;
	struct tnfs_track_data *tracknode2;

	changed = 0;

	if (*current != -1) {
		do {
			node = *current;

			tracknode1 = &track_data[node];
			tracknode2 = &track_data[node + 1];
			position.x = (tracknode1->pos.x + tracknode2->pos.x) >> 1;
			position.z = (tracknode1->pos.z + tracknode2->pos.z) >> 1;
			dist1 = math_vec3_distance_squared_XZ(&position, &car_data->position);

			tracknode1 = &track_data[node + 1];
			tracknode2 = &track_data[node + 2];
			position.x = (tracknode1->pos.x + tracknode2->pos.x) >> 1;
			position.z = (tracknode1->pos.z + tracknode2->pos.z) >> 1;
			dist2 = math_vec3_distance_squared_XZ(&position, &car_data->position);

			if (dist2 < dist1) {
				changed = 1;
				*current = *current + 1;
			} else if (0 < *current) {
				tracknode1 = &track_data[node - 1];
				tracknode2 = &track_data[node];
				position.x = (tracknode1->pos.x + tracknode2->pos.x) >> 1;
				position.z = (tracknode1->pos.z + tracknode2->pos.z) >> 1;
				dist2 = math_vec3_distance_squared_XZ(&position, &car_data->position);

				if (dist2 < dist1) {
					node = *current;
					*current = node - 1;
					if (node - 1 < 0) {
						*current = 0;
					} else {
						changed = 1;
					}
				}
			}
		} while (node != *current);
	}
	return changed;
}

int tnfs_road_segment_update(tnfs_car_data *car) {
	int changed;
	int segment;
	segment = car->road_segment_a;
	changed = tnfs_road_segment_find(car, &segment);
	car->road_segment_a = segment;
	car->road_segment_b = segment;
	return changed;
}

void tnfs_track_update_vectors(tnfs_car_data *car) {
	int node;
	tnfs_vec3 heading;
	tnfs_vec3 wall_normal;

	// current node
	node = car->road_segment_a;
	car->road_position.x = track_data[node].pos.x;
	car->road_position.y = track_data[node].pos.y;
	car->road_position.z = track_data[node].pos.z;

	wall_normal.x = track_data[node].wall_normal.x;
	wall_normal.y = track_data[node].wall_normal.y;
	wall_normal.z = track_data[node].wall_normal.z;

	// next node vector
	node++;
	heading.x = track_data[node].pos.x - car->road_position.x;
	heading.y = track_data[node].pos.y - car->road_position.y;
	heading.z = track_data[node].pos.z - car->road_position.z;

	math_vec3_normalize(&heading);

	// 0x10000, 0, 0 => points to right side of road
	car->road_fence_normal.x = wall_normal.x;
	car->road_fence_normal.y = wall_normal.y;
	car->road_fence_normal.z = wall_normal.z;

	// 0, 0x10000, 0 => up
	car->road_surface_normal.x = fixmul(heading.y, wall_normal.z) - fixmul(heading.z, wall_normal.y);
	car->road_surface_normal.y = fixmul(heading.z, wall_normal.x) - fixmul(heading.x, wall_normal.z);
	car->road_surface_normal.z = fixmul(heading.x, wall_normal.y) - fixmul(heading.y, wall_normal.x);

	// 0, 0, 0x10000 => north
	car->road_heading.x = heading.x;
	car->road_heading.y = heading.y;
	car->road_heading.z = heading.z;

	// ...
}

/*
 * setup everything
 */
void tnfs_init_sim(char * trifile) {
	cheat_crashing_cars = 0;
	sound_flag = 0;
	car_data.road_segment_a = 0;
	car_data.road_segment_b = 0;

	tnfs_init_track(trifile);
	tnfs_init_car();

	tnfs_reset_car();
}

/*
 * minimal basic main loop
 */
void tnfs_update() {
	// update camera
	switch (selected_camera) {
	case 1: //heli cam
		camera_position.x = car_data.position.x;
		camera_position.y = car_data.position.y + 0x60000;
		camera_position.z = car_data.position.z - 0x100000;
		break;
	default: //chase cam
		camera_position.x = car_data.position.x;
		camera_position.y = car_data.position.y + 0x50000;
		camera_position.z = car_data.position.z - 0x96000;
		break;
	}

	if (car_data.collision_data.crashed_time == 0) {
		// driving mode loop
		tnfs_driving_main();
		// update render matrix
		matrix_create_from_pitch_yaw_roll(&car_data.matrix, car_data.angle_x + car_data.body_pitch, car_data.angle_y, car_data.angle_z + car_data.body_roll);
	} else {
		// crash mode loop
		tnfs_collision_main(&car_data);
	}


	// tweak to allow circuit track lap
	if (car_data.road_segment_a == road_segment_count) {
		car_data.road_segment_a = 0;
	}

	int node = car_data.road_segment_a;
	car_data.road_ground_position.x = track_data[node].pos.x;
	car_data.road_ground_position.y = track_data[node].pos.y;
	car_data.road_ground_position.z = track_data[node].pos.z;
}
