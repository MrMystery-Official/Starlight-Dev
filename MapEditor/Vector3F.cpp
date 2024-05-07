#include "Vector3F.h"

#include <math.h>
#include <cmath>

Vector3F::Vector3F(float x, float y, float z)
{
	this->m_Data[0] = x;
	this->m_Data[1] = y;
	this->m_Data[2] = z;
}

void Vector3F::SetX(float x)
{
	this->m_Data[0] = x;
}

void Vector3F::SetY(float y)
{
	this->m_Data[1] = y;
}

void Vector3F::SetZ(float z)
{
	this->m_Data[2] = z;
}

float Vector3F::GetX()
{
	return this->m_Data[0];
}

float Vector3F::GetY()
{
	return this->m_Data[1];
}

float Vector3F::GetZ()
{
	return this->m_Data[2];
}

float* Vector3F::GetRawData()
{
	return this->m_Data;
}

float Vector3F::GetDotProduct()
{
	return this->GetX() + this->GetY() + this->GetZ();
}

float Vector3F::GetDistance()
{
	return std::sqrtf( std::powf(this->GetX(), 2) + std::powf(this->GetY(), 2) + std::powf(this->GetZ(), 2) );
}

bool Vector3F::operator==(Vector3F V) {
	return this->GetRawData()[0] == V.m_Data[0] && this->GetRawData()[1] == V.m_Data[1] && this->GetRawData()[2] == V.m_Data[2];
}

Vector3F Vector3F::operator-(Vector3F V)
{
	return Vector3F(this->GetX() - V.GetX(), this->GetY() - V.GetY(), this->GetZ() - V.GetZ());
}

Vector3F Vector3F::operator+(Vector3F V)
{
	return Vector3F(this->GetX() + V.GetX(), this->GetY() + V.GetY(), this->GetZ() + V.GetZ());
}

Vector3F Vector3F::operator*(Vector3F V)
{
	return Vector3F(this->GetX() * V.GetX(), this->GetY() * V.GetY(), this->GetZ() * V.GetZ());
}

Vector3F Vector3F::operator*(float Multiplier)
{
	return Vector3F(this->GetX() * Multiplier, this->GetY() * Multiplier, this->GetZ() * Multiplier);
}