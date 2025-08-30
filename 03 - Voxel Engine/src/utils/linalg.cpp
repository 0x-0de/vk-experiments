#include "linalg.h"

#include <algorithm>
#include <cmath>
#include <exception>
#include <iomanip>
#include <string>

static uint8_t permutations[] =
{
	21, 32, 54, 58, 56, 37, 36, 50,
	34, 14, 15,  6, 43, 59,  9, 13,
	46, 44, 24, 35, 10, 20, 53, 18,
	30, 21, 19,  8, 32, 11, 61, 26,
	22, 55,  0, 17, 28,  2, 23,  5,
	62,  3, 42, 49,  7, 41, 40, 60,
	51, 39, 33, 25, 16, 48, 29, 47,
	 4, 57, 63, 45, 52, 31,  1, 38
};

math::vec::vec()
{
	data_size = 0;
}

math::vec::vec(uint16_t size) : data_size(size)
{
	if(size != 0)
		data = new double[size];
	
	for(uint16_t i = 0; i < size; i++)
		data[i] = 0;
}

math::vec::vec(const vec& v)
{
	data_size = v.size();
	if(data_size != 0)
		data = new double[data_size];
	
	for(uint16_t i = 0; i < data_size; i++)
		data[i] = v[i];
}

math::vec::~vec()
{
	if(data_size != 0) delete[] data;
}

double math::vec::length() const
{
	if(data_size == 0) return 0;
	
	double len = 0;
	for(uint16_t i = 0; i < data_size; i++)
	{
		len += data[i] * data[i];
	}
	len = std::sqrt(len);
	
	return len;
}

double& math::vec::operator[](uint16_t index) const
{
	if(index >= data_size)
	{
		std::string exc = "Cannot access element " + std::to_string(index) + " from math::vec of size " + std::to_string(data_size) + ".";
		throw std::out_of_range(exc);
	}
	return data[index];
}

void math::vec::operator=(const vec& v)
{
	if(v.size() != data_size)
	{
		if(data_size != 0) delete[] data;
		data_size = v.size();
		data = new double[data_size];
	}
	
	for(size_t i = 0; i < data_size; i++)
	{
		data[i] = v[i];
	}
}

void math::vec::operator+=(const vec& v)
{
	if(v.size() != data_size)
	{
		std::string exc = "Cannot add math::vec of size " + std::to_string(v.size()) + " to math::vec of size " + std::to_string(data_size) + ".";
		throw std::range_error(exc);
	}
	
	for(uint16_t i = 0; i < data_size; i++)
	{
		data[i] += v[i];
	}
}

void math::vec::operator-=(const vec& v)
{
	if(v.size() != data_size)
	{
		std::string exc = "Cannot subtract math::vec of size " + std::to_string(v.size()) + " to math::vec of size " + std::to_string(data_size) + ".";
		throw std::range_error(exc);
	}
	
	for(uint16_t i = 0; i < data_size; i++)
	{
		data[i] -= v[i];
	}
}

void math::vec::operator*=(double m)
{
	for(uint16_t i = 0; i < data_size; i++)
	{
		data[i] *= m;
	}
}

void math::vec::operator/=(double m)
{
	if(m == 0)
	{
		throw std::invalid_argument("Cannot divide math::vec by 0.");
	}
	
	for(uint16_t i = 0; i < data_size; i++)
	{
		data[i] /= m;
	}
}

uint16_t math::vec::size() const
{
	return data_size;
}

math::mat::mat()
{
	size_m = 0;
	size_n = 0;
}

math::mat::mat(uint16_t m, uint16_t n) : size_m(m), size_n(n)
{
	if(m != 0)
	{
		data = new vec[m];
		if(n != 0)
		{
			for(uint16_t i = 0; i < m; i++)
				data[i] = vec(n);
		}
	}
		
	uint16_t mn = std::min(m, n);
	for(uint16_t i = 0; i < mn; i++)
		data[i][i] = 1;
}

math::mat::mat(const mat& ma)
{
	size_m = ma.size_m;
	size_n = ma.size_n;
	
	if(size_m != 0)
	{
		data = new vec[size_m];
		if(size_n != 0)
		{
			for(uint16_t i = 0; i < size_m; i++)
				data[i] = vec(size_n);
		}
	}
	
	for(uint16_t i = 0; i < size_m; i++)
	for(uint16_t j = 0; j < size_n; j++)
	{
		data[i][j] = ma.data[i][j];
	}
}

math::mat::~mat()
{
	if(size_m != 0)
	{
		delete[] data;
	}
}

void math::mat::get_data(float* dat)
{
	uint32_t index = 0;
	for(uint16_t i = 0; i < num_columns(); i++)
	for(uint16_t j = 0; j < num_rows(); j++)
	{
		dat[index] = data[i][j];
		index++;
	}
}

math::vec& math::mat::operator[](uint16_t index) const
{
	if(index >= size_m)
	{
		std::string exc = "Cannot access vector " + std::to_string(index) + " from math::mat of size " + std::to_string(size_m) + ".";
		throw std::out_of_range(exc);
	}
	
	return data[index];
}

void math::mat::operator=(const mat& m)
{
	if(size_m != m.size_m)
	{
		if(size_m != 0) delete[] data;
		size_m = m.size_m;
		size_n = m.size_n;
		data = new vec[size_m];
		for(uint16_t i = 0; i < size_m; i++)
			data[i] = vec(size_n);
	}
	else if(size_n != m.size_n)
	{
		size_n = m.size_n;
		for(uint16_t i = 0; i < size_m; i++)
			data[i] = vec(size_n);
	}
	
	for(uint16_t i = 0; i < size_m; i++)
	for(uint16_t j = 0; j < size_n; j++)
	{
		data[i][j] = m[i][j];
	}
}

void math::mat::operator+=(const mat& m)
{
	bool columns_equal = num_columns() == m.num_columns();
	bool rows_equal = num_rows() == m.num_rows();
	
	if(!columns_equal || !rows_equal)
	{
		std::string order_a = std::to_string(num_columns()) + "x" + std::to_string(num_rows());
		std::string order_b = std::to_string(m.num_columns()) + "x" + std::to_string(m.num_rows());
		std::string exc = "Cannot add math::mat of order " + order_b + " to math::mat of order " + order_a + ".";
		throw std::range_error(exc);
	}
	
	for(uint16_t i = 0; i < num_columns(); i++)
	for(uint16_t j = 0; j < num_rows(); j++)
	{
		data[i][j] += m[i][j];
	}
}

void math::mat::operator-=(const mat& m)
{
	bool columns_equal = num_columns() == m.num_columns();
	bool rows_equal = num_rows() == m.num_rows();
	
	if(!columns_equal || !rows_equal)
	{
		std::string order_a = std::to_string(num_columns()) + "x" + std::to_string(num_rows());
		std::string order_b = std::to_string(m.num_columns()) + "x" + std::to_string(m.num_rows());
		std::string exc = "Cannot subtract math::mat of order " + order_b + " to math::mat of order " + order_a + ".";
		throw std::range_error(exc);
	}
	
	for(uint16_t i = 0; i < num_columns(); i++)
	for(uint16_t j = 0; j < num_rows(); j++)
	{
		data[i][j] -= m[i][j];
	}
}

void math::mat::operator*=(const double& b)
{	
	for(uint16_t i = 0; i < num_columns(); i++)
	for(uint16_t j = 0; j < num_rows(); j++)
	{
		data[i][j] *= b;
	}
}

void math::mat::operator/=(const double& b)
{
	if(b == 0)
	{
		throw std::invalid_argument("Cannot divide math::mat by 0.");
	}
		
	for(uint16_t i = 0; i < num_columns(); i++)
	for(uint16_t j = 0; j < num_rows(); j++)
	{
		data[i][j] /= b;
	}
}

uint16_t math::mat::num_columns() const
{
	return size_m;
}

uint16_t math::mat::num_rows() const
{
	return size_n;
}

void math::mat::set(mat ma)
{
	if(ma.num_columns() != num_columns() || ma.num_rows() != num_rows())
	{
		std::string exc = "Cannot set math::mat of order " + std::to_string(size_m) + "x" + std::to_string(size_n) + " to math::mat of order " + std::to_string(ma.num_columns()) + "x" + std::to_string(ma.num_rows()) + ".";
		throw std::invalid_argument(exc);
	}
	
	for(uint16_t i = 0; i < num_columns(); i++)
	for(uint16_t j = 0; j < num_rows(); j++)
	{
		data[i][j] = ma[i][j];
	}
}

math::vec math::vec2(double x, double y)
{
	vec v(2);
	v[0] = x;
	v[1] = y;
	return v;
}

math::vec math::vec3(double x, double y, double z)
{
	vec v(3);
	v[0] = x;
	v[1] = y;
	v[2] = z;
	return v;
}

math::vec math::vec4(double x, double y, double z, double w)
{
	vec v(4);
	v[0] = x;
	v[1] = y;
	v[2] = z;
	v[3] = w;
	return v;
}

math::vec math::operator+(const vec& a, const vec& b)
{
	if(a.size() != b.size())
	{
		std::string exc = "Cannot perform add on math::vec of size " + std::to_string(a.size()) + " with math::vec of size " + std::to_string(b.size()) + ".";
		throw std::range_error(exc);
	}
	
	vec v(a.size());
	for(uint16_t i = 0; i < a.size(); i++)
	{
		v[i] = a[i] + b[i];
	}
	return v;
}

math::vec math::operator-(const vec& a, const vec& b)
{
	if(a.size() != b.size())
	{
		std::string exc = "Cannot perform subtract on math::vec of size " + std::to_string(a.size()) + " with math::vec of size " + std::to_string(b.size()) + ".";
		throw std::range_error(exc);
	}
	
	vec v(a.size());
	for(uint16_t i = 0; i < a.size(); i++)
	{
		v[i] = a[i] - b[i];
	}
	return v;
}

math::vec math::operator-(const vec& a)
{
	vec v(a.size());
	for(uint16_t i = 0; i < a.size(); i++)
	{
		v[i] = -a[i];
	}
	return v;
}

math::vec math::operator*(const vec& a, double m)
{
	vec v(a.size());
	for(uint16_t i = 0; i < a.size(); i++)
	{
		v[i] = a[i] * m;
	}
	return v;
}

math::vec math::operator/(const vec& a, double m)
{
	if(m == 0)
	{
		throw std::invalid_argument("Cannot divide math::vec by 0.");
	}
	
	vec v(a.size());
	for(uint16_t i = 0; i < a.size(); i++)
	{
		v[i] = a[i] / m;
	}
	return v;
}

math::mat math::operator+(const mat& a, const mat& b)
{
	bool columns_equal = a.num_columns() == b.num_columns();
	bool rows_equal = a.num_rows() == b.num_rows();
	
	if(!columns_equal || !rows_equal)
	{
		std::string order_a = std::to_string(a.num_columns()) + "x" + std::to_string(a.num_rows());
		std::string order_b = std::to_string(b.num_columns()) + "x" + std::to_string(b.num_rows());
		std::string exc = "Cannot perform add on math::mat of order " + order_a + " with math::mat of order " + order_b + ".";
		throw std::range_error(exc);
	}
		
	mat ma(a.num_columns(), a.num_rows());
		
	for(uint16_t i = 0; i < a.num_columns(); i++)
	for(uint16_t j = 0; j < a.num_rows(); j++)
	{
		ma[i][j] = a[i][j] + b[i][j];
	}
		
	return ma;
}

math::mat math::operator-(const mat& a, const mat& b)
{
	bool columns_equal = a.num_columns() == b.num_columns();
	bool rows_equal = a.num_rows() == b.num_rows();
	
	if(!columns_equal || !rows_equal)
	{
		std::string order_a = std::to_string(a.num_columns()) + "x" + std::to_string(a.num_rows());
		std::string order_b = std::to_string(b.num_columns()) + "x" + std::to_string(b.num_rows());
		std::string exc = "Cannot perform subtract on math::mat of order " + order_a + " with math::mat of order " + order_b + ".";
		throw std::range_error(exc);
	}
	
	mat ma(a.num_columns(), a.num_rows());
	
	for(uint16_t i = 0; i < a.num_columns(); i++)
	for(uint16_t j = 0; j < a.num_rows(); j++)
	{
		ma[i][j] = a[i][j] - b[i][j];
	}
	
	return ma;
}

math::mat math::operator*(const mat& a, const double& b)
{
	mat ma(a.num_columns(), a.num_rows());
	
	for(uint16_t i = 0; i < a.num_columns(); i++)
	for(uint16_t j = 0; j < a.num_rows(); j++)
	{
		ma[i][j] = a[i][j] * b;
	}
	
	return ma;
}

math::mat math::operator/(const mat& a, const double& b)
{
	if(b == 0)
	{
		throw std::invalid_argument("Cannot divide math::mat by 0.");
	}
	
	mat ma(a.num_columns(), a.num_rows());
	
	for(uint16_t i = 0; i < a.num_columns(); i++)
	for(uint16_t j = 0; j < a.num_rows(); j++)
	{
		ma[i][j] = a[i][j] / b;
	}
	
	return ma;
}

math::mat math::operator*(const mat& a, const mat& b)
{
	if(a.num_columns() != b.num_rows())
	{
		std::string order_a = std::to_string(a.num_columns()) + "x" + std::to_string(a.num_rows());
		std::string order_b = std::to_string(b.num_columns()) + "x" + std::to_string(b.num_rows());
		std::string exc = "Cannot perform multiply on math::mat of order " + order_a + " with math::mat of order " + order_b + ".";
		throw std::invalid_argument(exc);
	}
	
	mat ma(b.num_columns(), a.num_rows());
	
	for(uint32_t i = 0; i < ma.num_columns(); i++)
	for(uint32_t j = 0; j < ma.num_rows(); j++)
	{
		double val = 0;
		for(uint32_t k = 0; k < a.num_columns(); k++)
		{
			val += a[k][j] * b[i][k];
		}
		ma[i][j] = val;
	}
	
	return ma;
}

double math::dot(const vec& a, const vec& b)
{
	if(a.size() != b.size())
	{
		std::string exc = "Cannot find dot product of math::vec of size " + std::to_string(a.size()) + " and math::vec of size " + std::to_string(b.size()) + ".";
		throw std::invalid_argument(exc);
	}
	
	double val = 0;
	for(uint16_t i = 0; i < a.size(); i++)
	{
		val += a[i] * b[i];
	}
	return val;
}

math::vec math::cross(const vec& a, const vec& b)
{
	if(a.size() != b.size() || a.size() != 3)
	{
		std::string exc = "Cannot find cross product of math::vec of size " + std::to_string(a.size()) + " and math::vec of size " + std::to_string(b.size()) + ".";
		throw std::invalid_argument(exc);
	}
	
	vec v(3);
	v[0] = a[1] * b[2] - a[2] * b[1];
	v[1] = a[2] * b[0] - a[0] * b[2];
	v[2] = a[0] * b[1] - a[1] * b[0];
	return v;
}

math::vec math::normalize(const vec& a)
{
	if(a.length() == 0)
	{
		std::string exc = "Cannot normalize vector of length 0.";
		throw std::range_error(exc);
	}

	return a / a.length();
}

std::ostream& math::operator<<(std::ostream& os, const vec& v)
{
	os << "(";
	uint16_t size = v.size();
	for(uint16_t i = 0; i < size; i++)
	{
		os << v[i];
		if(i < size - 1) os << ", ";
	}
	os << ")";
	return os;
}

std::ostream& math::operator<<(std::ostream& os, const mat& m)
{
	os << "[\n";
	uint16_t columns = m.num_columns();
	uint16_t rows = m.num_rows();
	for(uint16_t j = 0; j < rows; j++)
	{
		os << "  [";
		for(uint16_t i = 0; i < columns; i++)
		{
			char brace = i < columns - 1 ? '|' : ']';
			os << std::setw(5) << std::setfill(' ') << m[i][j] << brace;
		}
		os << "\n";
	}
	os << "]";
	return os;
}

math::mat math::rotation(vec axis_angles)
{
	mat rot_x(3, 3);
	mat rot_y(3, 3);
	mat rot_z(3, 3);
	
	rot_x[1][1] = std::cos(axis_angles[0]);
	rot_x[2][1] = -std::sin(axis_angles[0]);
	rot_x[1][2] = std::sin(axis_angles[0]);
	rot_x[2][2] = std::cos(axis_angles[0]);
	
	rot_y[0][0] = std::cos(axis_angles[1]);
	rot_y[2][0] = -std::sin(axis_angles[1]);
	rot_y[0][2] = std::sin(axis_angles[1]);
	rot_y[2][2] = std::cos(axis_angles[1]);
	
	rot_z[0][0] = std::cos(axis_angles[2]);
	rot_z[1][0] = -std::sin(axis_angles[2]);
	rot_z[0][1] = std::sin(axis_angles[2]);
	rot_z[1][1] = std::cos(axis_angles[2]);
	
	return rot_x * rot_y * rot_z;
}

math::mat math::transform(vec translation, mat rotation, vec scale)
{
	if(translation.size() != 3)
	{
		throw std::invalid_argument("Invalid translation vector. Must be of size 3.");
	}
	
	if(rotation.num_columns() != 3 && rotation.num_rows() != 3)
	{
		throw std::invalid_argument("Invalid rotation matrix. Must be of order 3x3.");
	}
	
	if(scale.size() != 3)
	{
		throw std::invalid_argument("Invalid scale vector. Must be of size 3.");
	}
	
	mat rot(4, 4);
	
	for(uint16_t i = 0; i < 3; i++)
	for(uint16_t j = 0; j < 3; j++)
	{
		rot[i][j] = rotation[i][j];
	}
	
	mat scl(4, 4);
	scl[0][0] = scale[0];
	scl[1][1] = scale[1];
	scl[2][2] = scale[2];
	
	mat pos(4, 4);
	pos[3][0] = translation[0];
	pos[3][1] = translation[1];
	pos[3][2] = translation[2];
	
	return (rot * scl) * pos;
}

math::mat math::perspective(double fov, double aspect, double near_plane, double far_plane)
{
	mat proj(4, 4);

	double f = 1 / std::tan(fov / 2);

	proj[0][0] = f / aspect;
	proj[1][1] = f;
	proj[2][2] = (far_plane + near_plane) / (near_plane - far_plane);
	proj[3][2] = 2 * far_plane * near_plane / (near_plane - far_plane);
	proj[2][3] = -1;
	proj[3][3] = 0;

	return proj;
}

math::mat math::look_at(vec pos, vec target, vec base_up)
{
	vec z_axis = normalize(pos - target);
	vec x_axis = normalize(cross(-base_up, z_axis)); //We flip the y-axis because Vulkan associates +Y with down and -Y with up by default.
	vec y_axis = cross(z_axis, x_axis);

	mat look(4, 4);

	for(int i = 0; i < 3; i++)
	{
		look[i][0] = x_axis[i];
		look[i][1] = y_axis[i];
		look[i][2] = z_axis[i];
	}

	look[3][0] = -dot(pos, x_axis);
	look[3][1] = -dot(pos, y_axis);
	look[3][2] = -dot(pos, z_axis);

	return look;
}

uint64_t wraparound_right(uint64_t x, uint8_t shift)
{
	uint64_t r = x >> shift;
	r |= (x << (64 - shift));
	return r;
}

int64_t math::random(int64_t seed)
{
	int perm_mask = 63;
	int64_t r = seed + 0xc22dbcb72481193b;
	r = wraparound_right(r, permutations[seed & perm_mask]);
	for(int i = 0; i < 3; i++)
	{
		int8_t p = permutations[(unsigned) (r * r) & perm_mask];
		int8_t sp = permutations[(unsigned) (r + p) & perm_mask];
		r += r * p * (sp + 11);
		r *= ((r * r) % 23) + 1;
		p = wraparound_right(p, sp);
		r ^= (p * p * r);
		r = wraparound_right(r, permutations[(unsigned) r & perm_mask]);
	}
	r = wraparound_right(r, permutations[r & perm_mask]);
	return r;
}

double math::random_float(int64_t seed)
{
	return (double) random(seed) / INT64_MAX;
}

double math::interp_linear_1d(double a, double b, double t)
{
	return t * (b - a) + a;
}

double math::interp_linear_2d(double aa, double ba, double ab, double bb, double t, double tt)
{
	return interp_linear_1d(interp_linear_1d(aa, ba, t), interp_linear_1d(ab, bb, t), tt);
}

double math::interp_linear_3d(double aaa, double baa, double aba, double bba, double aab, double bab, double abb, double bbb, double t, double tt, double ttt)
{
	return interp_linear_1d(interp_linear_2d(aaa, baa, aba, bba, t, tt), interp_linear_2d(aab, bab, abb, bbb, t, tt), ttt);
}

double math::interp_cosine_1d(double a, double b, double t)
{
	double mu = (1 - std::cos(t * 3.1415926)) / 2;
	return (a * (1 - mu) + b * mu);
}

double math::interp_cosine_2d(double aa, double ba, double ab, double bb, double t, double tt)
{
	return interp_cosine_1d(interp_cosine_1d(aa, ba, t), interp_cosine_1d(ab, bb, t), tt);
}

double math::interp_cosine_3d(double aaa, double baa, double aba, double bba, double aab, double bab, double abb, double bbb, double t, double tt, double ttt)
{
	return interp_cosine_1d(interp_cosine_2d(aaa, baa, aba, bba, t, tt), interp_cosine_2d(aab, bab, abb, bbb, t, tt), ttt);
}

int64_t get_gradient_code_2d(int64_t x, int64_t y)
{
	return x + y + x * y + y * y;
}

int64_t get_gradient_code_3d(int64_t x, int64_t y, int64_t z)
{
	return x + y + z + x * y + y * z + z * x + y * y + z * z * z;
}

double math::gradient_noise_2d_linear(int64_t seed, double x, double y)
{
	int64_t ax = std::floor(x);
	int64_t ay = std::floor(y);
	
	int64_t bx = ax + 1;
	int64_t by = ay + 1;
	
	double dx = x - ax;
	double dy = y - ay;
	
	int64_t seed_aa = (get_gradient_code_2d(ax, ay) + seed);
	int64_t seed_ab = (get_gradient_code_2d(ax, by) + seed);
	int64_t seed_ba = (get_gradient_code_2d(bx, ay) + seed);
	int64_t seed_bb = (get_gradient_code_2d(bx, by) + seed);
	
	double r_aa = random_float(seed_aa);
	double r_ab = random_float(seed_ab);
	double r_ba = random_float(seed_ba);
	double r_bb = random_float(seed_bb);
	
	return interp_linear_2d(r_aa, r_ba, r_ab, r_bb, dx, dy);
}

double math::gradient_noise_3d_linear(int64_t seed, double x, double y, double z)
{
	int64_t ax = std::floor(x);
	int64_t ay = std::floor(y);
	int64_t az = std::floor(z);
	
	int64_t bx = ax + 1;
	int64_t by = ay + 1;
	int64_t bz = az + 1;
	
	double dx = x - ax;
	double dy = y - ay;
	double dz = z - az;
	
	int64_t seed_aaa = (get_gradient_code_3d(ax, ay, az) + seed);
	int64_t seed_baa = (get_gradient_code_3d(bx, ay, az) + seed);
	int64_t seed_aba = (get_gradient_code_3d(ax, by, az) + seed);
	int64_t seed_bba = (get_gradient_code_3d(bx, by, az) + seed);
	int64_t seed_aab = (get_gradient_code_3d(ax, ay, bz) + seed);
	int64_t seed_bab = (get_gradient_code_3d(bx, ay, bz) + seed);
	int64_t seed_abb = (get_gradient_code_3d(ax, by, bz) + seed);
	int64_t seed_bbb = (get_gradient_code_3d(bx, by, bz) + seed);
	
	double r_aaa = random_float(seed_aaa);
	double r_baa = random_float(seed_baa);
	double r_aba = random_float(seed_aba);
	double r_bba = random_float(seed_bba);
	double r_aab = random_float(seed_aab);
	double r_bab = random_float(seed_bab);
	double r_abb = random_float(seed_abb);
	double r_bbb = random_float(seed_bbb);
	
	return interp_linear_3d(r_aaa, r_baa, r_aba, r_bba, r_aab, r_bab, r_abb, r_bbb, dx, dy, dz);
}

double math::gradient_noise_2d_cosine(int64_t seed, double x, double y)
{
	int64_t ax = std::floor(x);
	int64_t ay = std::floor(y);
	
	int64_t bx = ax + 1;
	int64_t by = ay + 1;
	
	double dx = x - ax;
	double dy = y - ay;
	
	int64_t seed_aa = (get_gradient_code_2d(ax, ay) + seed);
	int64_t seed_ab = (get_gradient_code_2d(ax, by) + seed);
	int64_t seed_ba = (get_gradient_code_2d(bx, ay) + seed);
	int64_t seed_bb = (get_gradient_code_2d(bx, by) + seed);
	
	double r_aa = random_float(seed_aa);
	double r_ab = random_float(seed_ab);
	double r_ba = random_float(seed_ba);
	double r_bb = random_float(seed_bb);
	
	return interp_cosine_2d(r_aa, r_ba, r_ab, r_bb, dx, dy);
}

double math::gradient_noise_3d_cosine(int64_t seed, double x, double y, double z)
{
	int64_t ax = std::floor(x);
	int64_t ay = std::floor(y);
	int64_t az = std::floor(z);
	
	int64_t bx = ax + 1;
	int64_t by = ay + 1;
	int64_t bz = az + 1;
	
	double dx = x - ax;
	double dy = y - ay;
	double dz = z - az;
	
	int64_t seed_aaa = (get_gradient_code_3d(ax, ay, az) + seed);
	int64_t seed_baa = (get_gradient_code_3d(bx, ay, az) + seed);
	int64_t seed_aba = (get_gradient_code_3d(ax, by, az) + seed);
	int64_t seed_bba = (get_gradient_code_3d(bx, by, az) + seed);
	int64_t seed_aab = (get_gradient_code_3d(ax, ay, bz) + seed);
	int64_t seed_bab = (get_gradient_code_3d(bx, ay, bz) + seed);
	int64_t seed_abb = (get_gradient_code_3d(ax, by, bz) + seed);
	int64_t seed_bbb = (get_gradient_code_3d(bx, by, bz) + seed);
	
	double r_aaa = random_float(seed_aaa);
	double r_baa = random_float(seed_baa);
	double r_aba = random_float(seed_aba);
	double r_bba = random_float(seed_bba);
	double r_aab = random_float(seed_aab);
	double r_bab = random_float(seed_bab);
	double r_abb = random_float(seed_abb);
	double r_bbb = random_float(seed_bbb);
	
	return interp_cosine_3d(r_aaa, r_baa, r_aba, r_bba, r_aab, r_bab, r_abb, r_bbb, dx, dy, dz);
}