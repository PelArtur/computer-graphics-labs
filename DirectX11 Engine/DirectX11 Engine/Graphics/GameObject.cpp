#include "GameObject.hpp"


const XMVECTOR& GameObject::GetPositionVector() const
{
	return this->posVector;
}


const XMFLOAT3& GameObject::GetPositionFloat3() const
{
	return this->pos;
}


const XMVECTOR& GameObject::GetRotationVector() const
{
	return this->rotVector;
}


const XMFLOAT3& GameObject::GetRotationFloat3() const
{
	return this->rot;
}


const XMVECTOR& GameObject::GetForwardVector(bool omitY)
{
	return omitY ? this->vec_forward_noY : this->vec_forward;
}


const XMVECTOR& GameObject::GetRightVector(bool omitY)
{
	return omitY ? this->vec_right_noY : this->vec_right;
}


const XMVECTOR& GameObject::GetBackwardVector(bool omitY)
{
	return omitY ? this->vec_backward_noY : this->vec_backward;
}


const XMVECTOR& GameObject::GetLeftVector(bool omitY)
{
	return omitY ? this->vec_left_noY : this->vec_left;
}


void GameObject::SetPosition(const XMVECTOR& pos)
{
	XMStoreFloat3(&this->pos, pos);
	this->posVector = pos;
	this->checkPosition();
	this->UpdateMatrix();
}


void GameObject::SetPosition(const XMFLOAT3& pos)
{
	this->pos = pos;
	this->posVector = XMLoadFloat3(&this->pos);
	this->checkPosition();
	this->UpdateMatrix();
}


void GameObject::SetPosition(float x, float y, float z)
{
	this->pos = XMFLOAT3(x, y, z);
	this->posVector = XMLoadFloat3(&this->pos);
	this->checkPosition();
	this->UpdateMatrix();
}


void GameObject::AdjustPosition(const XMVECTOR& pos)
{
	this->posVector += pos;
	XMStoreFloat3(&this->pos, this->posVector);
	this->checkPosition();
	this->UpdateMatrix();
}


void GameObject::AdjustPosition(const XMFLOAT3& pos)
{
	this->pos.x += pos.x;
	this->pos.y += pos.y;
	this->pos.z += pos.z;
	this->posVector = XMLoadFloat3(&this->pos);
	this->checkPosition();
	this->UpdateMatrix();
}


void GameObject::AdjustPosition(float x, float y, float z)
{
	this->pos.x += x;
	this->pos.y += y;
	this->pos.z += z;
	this->posVector = XMLoadFloat3(&this->pos);
	this->checkPosition();
	this->UpdateMatrix();
}


void GameObject::SetRotation(const XMVECTOR& rot)
{
	this->rotVector = rot;
	XMStoreFloat3(&this->rot, rot);
	this->checkRotation();
	this->UpdateMatrix();
}

void GameObject::SetRotation(const XMFLOAT3& rot)
{
	this->rot = rot;
	this->rotVector = XMLoadFloat3(&this->rot);
	this->checkRotation();
	this->UpdateMatrix();
}


void GameObject::SetRotation(float x, float y, float z)
{
	this->rot = XMFLOAT3(x, y, z);
	this->rotVector = XMLoadFloat3(&this->rot);
	this->checkRotation();
	this->UpdateMatrix();
}


void GameObject::AdjustRotation(const XMVECTOR& rot)
{
	this->rotVector += rot;
	XMStoreFloat3(&this->rot, this->rotVector);
	this->checkRotation();
	this->UpdateMatrix();
}


void GameObject::AdjustRotation(const XMFLOAT3& rot)
{
	this->rot.x += rot.x;
	this->rot.y += rot.y;
	this->rot.z += rot.z;
	this->rotVector = XMLoadFloat3(&this->rot);
	this->checkRotation();
	this->UpdateMatrix();
}


void GameObject::AdjustRotation(float x, float y, float z)
{
	this->rot.x += x;
	this->rot.y += y;
	this->rot.z += z;
	this->rotVector = XMLoadFloat3(&this->rot);
	this->checkRotation();
	this->UpdateMatrix();
}


void GameObject::SetLookAtPos(XMFLOAT3 lookAtPos)
{
	if (lookAtPos.x == this->pos.x && lookAtPos.y == this->pos.y && lookAtPos.z == this->pos.z)
		return;

	lookAtPos.x = this->pos.x - lookAtPos.x;
	lookAtPos.y = this->pos.y - lookAtPos.y;
	lookAtPos.z = this->pos.z - lookAtPos.z;

	float pitch = 0.0f;
	if (lookAtPos.y != 0.0f)
	{
		const float distance = sqrt(lookAtPos.x * lookAtPos.x + lookAtPos.z * lookAtPos.z);
		pitch = atan(lookAtPos.y / distance);
	}

	float yaw = 0.0f;
	if (lookAtPos.x != 0.0f)
		yaw = atan(lookAtPos.x / lookAtPos.z);
	if (lookAtPos.z > 0)
		yaw += XM_PI;

	this->checkRotation();
	this->SetRotation(pitch, yaw, 0.0f);
}


void GameObject::UpdateMatrix()
{
	assert("UpdateMatrix must be overridden." && 0);
}

void GameObject::UpdateDirectionVectors()
{
	XMMATRIX vecRotationMatrix = XMMatrixRotationRollPitchYaw(this->rot.x, this->rot.y, 0.0f);
	this->vec_forward = XMVector3TransformCoord(this->DEFAULT_FORWARD_VECTOR, vecRotationMatrix);
	this->vec_backward = XMVector3TransformCoord(this->DEFAULT_BACKWARD_VECTOR, vecRotationMatrix);
	this->vec_left = XMVector3TransformCoord(this->DEFAULT_LEFT_VECTOR, vecRotationMatrix);
	this->vec_right = XMVector3TransformCoord(this->DEFAULT_RIGHT_VECTOR, vecRotationMatrix);

	XMMATRIX vecRotationMatrixnoY = XMMatrixRotationRollPitchYaw(0.0f, this->rot.y, 0.0f);
	this->vec_forward_noY = XMVector3TransformCoord(this->DEFAULT_FORWARD_VECTOR, vecRotationMatrixnoY);
	this->vec_backward_noY = XMVector3TransformCoord(this->DEFAULT_BACKWARD_VECTOR, vecRotationMatrixnoY);
	this->vec_left_noY = XMVector3TransformCoord(this->DEFAULT_LEFT_VECTOR, vecRotationMatrixnoY);
	this->vec_right_noY = XMVector3TransformCoord(this->DEFAULT_RIGHT_VECTOR, vecRotationMatrixnoY);
}


void GameObject::checkPosition() { return; }
void GameObject::checkRotation() { return; }
