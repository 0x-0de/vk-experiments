#ifndef _LINALG_H_
#define _LINALG_H_

#include <cstdint>
#include <iostream>

namespace math
{
	class vec
	{
		public:
			vec();
			vec(uint16_t size);
			vec(const vec& v);
			~vec();
			
			double length() const;
			
			double& operator[](uint16_t index) const;
			
			void operator=(const vec& v);
			
			void operator+=(const vec& v);
			void operator-=(const vec& v);
			void operator*=(double m);
			void operator/=(double m);
			
			uint16_t size() const;
		private:
			uint16_t data_size;
			double* data;
	};
	
	class mat
	{
		public:
			mat();
			mat(uint16_t m, uint16_t n);
			mat(const mat& ma);
			~mat();
			
			void get_data(float* dat);
			
			vec& operator[](uint16_t index) const;
			
			void operator=(const mat& m);
			
			void operator+=(const mat& m);
			void operator-=(const mat& m);
			void operator*=(const double& b);
			void operator/=(const double& b);
			
			uint16_t num_columns() const;
			uint16_t num_rows() const;
		private:
			uint16_t size_m, size_n;
			vec* data;
	};
	
	vec vec2(double x, double y);
	vec vec3(double x, double y, double z);
	vec vec4(double x, double y, double z, double w);
	
	vec operator+(const vec& a, const vec& b);
	vec operator-(const vec& a, const vec& b);
	vec operator-(const vec& a);
	vec operator*(const vec& v, double m);
	vec operator/(const vec& v, double m);
	
	mat operator+(const mat& a, const mat& b);
	mat operator-(const mat& a, const mat& b);
	mat operator*(const mat& a, const double& b);
	mat operator/(const mat& a, const double& b);
	mat operator*(const mat& a, const mat& b);
	
	double dot(const vec& a, const vec& b);
	vec cross(const vec& a, const vec& b);
	vec normalize(const vec& a);
	
	std::ostream& operator<<(std::ostream& os, const vec& v);
	std::ostream& operator<<(std::ostream& os, const mat& m);
	
	mat rotation(vec axis_angles);
	
	mat transform(vec translation, mat rotation, vec scale);
	mat perspective(double fov, double aspect, double near_plane, double far_plane);
	mat look_at(vec pos, vec target, vec base_up);
}

#endif