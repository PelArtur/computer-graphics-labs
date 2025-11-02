#pragma once
#include "Model.hpp"

class GameObject
{
public:
	const XMVECTOR& GetPositionVector() const;
	const XMFLOAT3& GetPositionFloat3() const;
	const XMVECTOR& GetRotationVector() const;
	const XMFLOAT3& GetRotationFloat3() const;
	const XMVECTOR& GetForwardVector(bool omitY = false);
	const XMVECTOR& GetRightVector(bool omitY = false);
	const XMVECTOR& GetBackwardVector(bool omitY = false);
	const XMVECTOR& GetLeftVector(bool omitY = false);

	void SetPosition(const XMVECTOR& pos);
	void SetPosition(const XMFLOAT3& pos);
	void SetPosition(float x, float y, float z);
	void AdjustPosition(const XMVECTOR& pos);
	void AdjustPosition(const XMFLOAT3& pos);
	void AdjustPosition(float x, float y, float z);
	void SetRotation(const XMVECTOR& rot);
	void SetRotation(const XMFLOAT3& rot);
	void SetRotation(float x, float y, float z);
	void AdjustRotation(const XMVECTOR& rot);
	void AdjustRotation(const XMFLOAT3& rot);
	void AdjustRotation(float x, float y, float z);
	void SetLookAtPos(XMFLOAT3 lookAtPos);
	void SetScale(const XMVECTOR& scl);
	void SetScale(const XMFLOAT3& scl);
	void SetScale(float x, float y, float z);
	void AdjustScale(const XMVECTOR& scl);
	void AdjustScale(const XMFLOAT3& scl);
	void AdjustScale(float x, float y, float z);

protected:
	virtual void UpdateMatrix();
	void UpdateDirectionVectors();
	virtual void checkPosition();
	virtual void checkRotation();

	XMVECTOR posVector;
	XMVECTOR rotVector;
	XMVECTOR sclVector;
	XMFLOAT3 pos;
	XMFLOAT3 rot;
	XMFLOAT3 scl;

	//left handed Y-up format
	const XMVECTOR DEFAULT_FORWARD_VECTOR = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
	const XMVECTOR DEFAULT_UP_VECTOR = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	const XMVECTOR DEFAULT_BACKWARD_VECTOR = XMVectorSet(0.0f, 0.0f, -1.0f, 0.0f);
	const XMVECTOR DEFAULT_LEFT_VECTOR = XMVectorSet(-1.0f, 0.0f, 0.0f, 0.0f);
	const XMVECTOR DEFAULT_RIGHT_VECTOR = XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);

	XMVECTOR vec_forward;
	XMVECTOR vec_left;
	XMVECTOR vec_right;
	XMVECTOR vec_backward;

	XMVECTOR vec_forward_noY;
	XMVECTOR vec_left_noY;
	XMVECTOR vec_right_noY;
	XMVECTOR vec_backward_noY;
};
