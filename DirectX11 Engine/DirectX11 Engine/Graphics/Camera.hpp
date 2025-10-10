#pragma once
#include "GameObject.hpp"
using namespace DirectX;


class Camera : public GameObject
{
public:
	Camera();
	void SetProjectionValues(float fovDegrees, float aspectRatio, float nearZ, float farZ);

	const XMMATRIX& GetViewMatrix() const;
	const XMMATRIX& GetProjectionMatrix() const;

private:
	void UpdateMatrix() override;
	void checkRotation() override;

	XMMATRIX viewMatrix;
	XMMATRIX projectionMatrix;
};
