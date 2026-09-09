#include "Light.hpp"

bool Light::Initialize(ID3D11Device* device, ID3D11DeviceContext* deviceContext, ConstantBuffer<CB_VS_vertexShader>& cb_vertexShader, LightType type)
{
	if (!model.Initialize("Data/Models/Light Ball/white_ball.glb", device, deviceContext, cb_vertexShader))
		return false;

	this->type = type;
	this->SetPosition(0.0f, 0.0f, 0.0f);
	this->SetRotation(0.0f, 0.0f, 0.0f);
	this->SetScale(1.0f, 1.0f, 1.0f);
	this->UpdateMatrix();
	return true;
}


void Light::Draw(const XMMATRIX& viewProjectionMatrix)
{
	this->SetPosition(lightPosition);
	model.Draw(this->worldMatrix, viewProjectionMatrix);
}


XMMATRIX CalculateDirectionalLightVP(XMFLOAT3 direction)
{
	XMFLOAT3 ld = direction;
	ld.y *= -1.0f;
	XMVECTOR lightDir = XMVector3Normalize(
		XMLoadFloat3(&ld)
	);
	XMVECTOR helper = XMVectorSet(0, 1, 0, 0);

	if (fabsf(XMVectorGetX(XMVector3Dot(lightDir, helper))) > 0.99f)
	{
		helper = XMVectorSet(0, 0, 1, 0);
	}

	XMVECTOR right = XMVector3Normalize(XMVector3Cross(helper, lightDir));
	XMVECTOR up = XMVector3Cross(lightDir, right);

	XMVECTOR center = XMVectorZero();

	float lightDistance = 200.0f;
	XMVECTOR lightPos = center - lightDir * lightDistance;

	XMMATRIX lightView = XMMATRIX(
		right,
		up,
		lightDir,
		XMVectorSet(
			-XMVectorGetX(XMVector3Dot(right, lightPos)),
			-XMVectorGetX(XMVector3Dot(up, lightPos)),
			-XMVectorGetX(XMVector3Dot(lightDir, lightPos)),
			1.0f
		)
	);

	float orthoSize = 40.0f;
	float nearZ = 0.1f;
	float farZ = 300.0f;

	XMMATRIX lightProj = XMMatrixOrthographicLH(
		orthoSize * 2.0f,
		orthoSize * 2.0f,
		nearZ,
		farZ
	);

	return lightView * lightProj;
}


XMMATRIX CalculateSpotlightVP(const LightData& light)
{
	XMVECTOR lightPos = XMLoadFloat3(&light.position);
	XMVECTOR lightDir = XMVector3Normalize(XMLoadFloat3(&light.direction));

	XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	if (fabsf(XMVectorGetX(XMVector3Dot(lightDir, up))) > 0.99f)
	{
		up = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
	}

	XMMATRIX lightView = XMMatrixLookToLH(
		lightPos,
		lightDir,
		up
	);

	float fovAngle = light.spotOuterAngle * 1.1f;
	float aspectRatio = 1.0f;

	float fovWithPadding = fovAngle * 2.0f;

	XMMATRIX lightProj = XMMatrixPerspectiveFovLH(
		fovWithPadding,
		aspectRatio,
		0.1f,
		100.0f
	);

	return lightView * lightProj;
}