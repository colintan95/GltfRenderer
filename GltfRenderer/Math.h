#pragma once

struct Vec3 {
	float x;
	float y;
	float z;

	Vec3();
	Vec3(float x, float y, float z);
};

struct Mat4 {
	float r0[4];
	float r1[4];
	float r2[4];
	float r3[4];

	Mat4();

	static Mat4 Identity();

	static Mat4 Translate(float x, float y, float z);

	static Mat4 RotateXZ(float angle);
	static Mat4 RotateYZ(float angle);

	static Mat4 Projection(float fov, float aspect, float zNear, float zFar);
};

Mat4 operator*(const Mat4& M0, const Mat4& M1);

Vec3 operator*(const Vec3& v, const Mat4& M);