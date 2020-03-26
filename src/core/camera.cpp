#include "camera.h"
#include "../util/serviceprovider.h"

using namespace DirectX;

Camera::Camera()
{
	setLens();

	yAxis = XMVectorSet(0.f, 1.0f, 0.f, 0.f);
}

XMVECTOR Camera::getPosition()const
{
    return XMLoadFloat3(&mPosition);
}

XMFLOAT3 Camera::getPosition3f()const
{
    return mPosition;
}

void Camera::setPosition(float x, float y, float z)
{
	mPosition = XMFLOAT3(x, y, z);
	mViewDirty = true;
}

void Camera::setPosition(const XMFLOAT3& v)
{
	mPosition = v;
	mViewDirty = true;
}

XMVECTOR Camera::getRight()const
{
	return XMLoadFloat3(&mRight);
}

XMFLOAT3 Camera::getRight3f()const
{
	return mRight;
}

XMVECTOR Camera::getUp()const
{
	return XMLoadFloat3(&mUp);
}

XMFLOAT3 Camera::getUp3f()const
{
	return mUp;
}

XMVECTOR Camera::getLook()const
{
	return XMLoadFloat3(&mLook);
}

XMFLOAT3 Camera::getLook3f()const
{
	return mLook;
}

float Camera::getNearZ()const
{
	return mNearZ;
}

float Camera::getFarZ()const
{
	return mFarZ;
}

float Camera::getAspect()const
{
	return mAspect;
}

float Camera::getFovY()const
{
	return mFovY;
}

float Camera::getFovX()const
{
	float halfWidth = 0.5f * getNearWindowWidth();
	return 2.0f * atan(halfWidth / mNearZ);
}

float Camera::getNearWindowWidth()const
{
	return mAspect * mNearWindowHeight;
}

float Camera::getNearWindowHeight()const
{
	return mNearWindowHeight;
}

float Camera::getFarWindowWidth()const
{
	return mAspect * mFarWindowHeight;
}

float Camera::getFarWindowHeight()const
{
	return mFarWindowHeight;
}

void Camera::setLens(float fovY, float aspect, float zn, float zf)
{
	// cache properties
	mFovY = fovY;
	mAspect = aspect;
	mNearZ = zn;
	mFarZ = zf;

	mNearWindowHeight = 2.0f * mNearZ * tanf(0.5f * mFovY);
	mFarWindowHeight = 2.0f * mFarZ * tanf(0.5f * mFovY);

	XMMATRIX P = XMMatrixPerspectiveFovLH(mFovY, mAspect, mNearZ, mFarZ);
	XMStoreFloat4x4(&mProj, P);
}

void Camera::setLens()
{
	float fovRad = ServiceProvider::getSettings()->displaySettings.FOV / 360.f * MathHelper::Pi;

	setLens(fovRad,
			static_cast<float>(ServiceProvider::getSettings()->displaySettings.ResolutionWidth) / ServiceProvider::getSettings()->displaySettings.ResolutionHeight,
			stdNear,
			stdFar);
}

void Camera::lookAt(FXMVECTOR pos, FXMVECTOR target, FXMVECTOR worldUp)
{
	XMVECTOR L = XMVector3Normalize(XMVectorSubtract(target, pos));
	XMVECTOR R = XMVector3Normalize(XMVector3Cross(worldUp, L));
	XMVECTOR U = XMVector3Cross(L, R);

	XMStoreFloat3(&mPosition, pos);
	XMStoreFloat3(&mLook, L);
	XMStoreFloat3(&mRight, R);
	XMStoreFloat3(&mUp, U);

	mViewDirty = true;
}

void Camera::lookAt(const XMFLOAT3& pos, const XMFLOAT3& target, const XMFLOAT3& up)
{
	XMVECTOR P = XMLoadFloat3(&pos);
	XMVECTOR T = XMLoadFloat3(&target);
	XMVECTOR U = XMLoadFloat3(&up);

	lookAt(P, T, U);

	mViewDirty = true;
}

XMMATRIX Camera::getView()const
{
	assert(!mViewDirty);
	return XMLoadFloat4x4(&mView);
}

XMMATRIX Camera::getProj()const
{
	return XMLoadFloat4x4(&mProj);
}


XMFLOAT4X4 Camera::getView4x4f()const
{
	assert(!mViewDirty);
	return mView;
}

XMFLOAT4X4 Camera::getProj4x4f()const
{
	return mProj;
}

void Camera::strafe(float d)
{
	// mPosition += d*mRight
	XMVECTOR s = XMVectorReplicate(d);
	XMVECTOR r = XMLoadFloat3(&mRight);
	XMVECTOR p = XMLoadFloat3(&mPosition);
	XMStoreFloat3(&mPosition, XMVectorMultiplyAdd(s, r, p));

	mViewDirty = true;
}

void Camera::walk(float d)
{
	// mPosition += d*mLook
	XMVECTOR s = XMVectorReplicate(d);
	XMVECTOR l = XMLoadFloat3(&mLook);
	XMVECTOR p = XMLoadFloat3(&mPosition);
	XMStoreFloat3(&mPosition, XMVectorMultiplyAdd(s, l, p));

	mViewDirty = true;
}

void Camera::pitch(float angle)
{
	// Rotate up and look vector about the right vector.

	XMMATRIX R = XMMatrixRotationAxis(XMLoadFloat3(&mRight), angle);

	XMVECTOR t = XMVector3TransformNormal(XMLoadFloat3(&mLook), R);
	XMVECTOR a = XMVector3AngleBetweenNormals(yAxis, t);

	float convertedAngle = XMConvertToDegrees(XMVectorGetX(a));

	if (convertedAngle < CAMERA_RESTRICTION_ANGLE || convertedAngle >(180.f - CAMERA_RESTRICTION_ANGLE))
		return;

	XMStoreFloat3(&mUp, XMVector3TransformNormal(XMLoadFloat3(&mUp), R));
	XMStoreFloat3(&mLook, XMVector3TransformNormal(XMLoadFloat3(&mLook), R));

	mViewDirty = true;
}

void Camera::rotateY(float angle)
{
	// Rotate the basis vectors about the world y-axis.

	XMMATRIX R = XMMatrixRotationY(angle);

	XMStoreFloat3(&mRight, XMVector3TransformNormal(XMLoadFloat3(&mRight), R));
	XMStoreFloat3(&mUp, XMVector3TransformNormal(XMLoadFloat3(&mUp), R));
	XMStoreFloat3(&mLook, XMVector3TransformNormal(XMLoadFloat3(&mLook), R));

	mViewDirty = true;
}

/*TODO*/
void Camera::roll(float angle)
{
	XMVECTOR X = XMLoadFloat3(&mLook);
	XMVECTOR R = XMVector3Transform(X, XMMatrixRotationZ(angle));
	XMMATRIX Y = XMMatrixRotationRollPitchYawFromVector(R);

	//XMMATRIX Y = XMMatrixRotationY(angle);

	//XMStoreFloat3(&mRight, XMVector3TransformNormal(XMLoadFloat3(&mRight), Y));
	//XMStoreFloat3(&mUp, XMVector3TransformNormal(XMLoadFloat3(&mUp), Y));
	XMStoreFloat3(&mLook, XMVector3TransformNormal(XMLoadFloat3(&mLook), XMMatrixRotationY(angle)));

	mViewDirty = true;
}

void Camera::updateViewMatrix()
{
	if (mViewDirty)
	{
		XMVECTOR R = XMLoadFloat3(&mRight);
		XMVECTOR U = XMLoadFloat3(&mUp);
		XMVECTOR L = XMLoadFloat3(&mLook);
		XMVECTOR P = XMLoadFloat3(&mPosition);

		// Keep camera's axes orthogonal to each other and of unit length.
		L = XMVector3Normalize(L);
		U = XMVector3Normalize(XMVector3Cross(L, R));

		// U, L already ortho-normal, so no need to normalize cross product.
		R = XMVector3Cross(U, L);

		// Fill in the view matrix entries.
		float x = -XMVectorGetX(XMVector3Dot(P, R));
		float y = -XMVectorGetX(XMVector3Dot(P, U));
		float z = -XMVectorGetX(XMVector3Dot(P, L));

		XMStoreFloat3(&mRight, R);
		XMStoreFloat3(&mUp, U);
		XMStoreFloat3(&mLook, L);

		mView(0, 0) = mRight.x;
		mView(1, 0) = mRight.y;
		mView(2, 0) = mRight.z;
		mView(3, 0) = x;

		mView(0, 1) = mUp.x;
		mView(1, 1) = mUp.y;
		mView(2, 1) = mUp.z;
		mView(3, 1) = y;

		mView(0, 2) = mLook.x;
		mView(1, 2) = mLook.y;
		mView(2, 2) = mLook.z;
		mView(3, 2) = z;

		mView(0, 3) = 0.0f;
		mView(1, 3) = 0.0f;
		mView(2, 3) = 0.0f;
		mView(3, 3) = 1.0f;

		mViewDirty = false;
	}
}