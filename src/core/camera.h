#pragma once

#include "../util/d3dUtil.h"

#define CAMERA_RESTRICTION_ANGLE 5.f
#define CAMERA_HITBOX_SIZE_EXTENTS 0.001f

class Camera
{
public:
    Camera();
    ~Camera() = default;

    DirectX::XMVECTOR getPosition()const;
    DirectX::XMFLOAT3 getPosition3f()const;
    void setPosition(float x, float y, float z);
    void setPosition(const DirectX::XMFLOAT3& v);

    DirectX::XMVECTOR getRight()const;
    DirectX::XMFLOAT3 getRight3f()const;
    DirectX::XMVECTOR getUp()const;
    DirectX::XMFLOAT3 getUp3f()const;
    DirectX::XMVECTOR getLook()const;
    DirectX::XMFLOAT3 getLook3f()const;

    float getNearZ()const;
    float getFarZ()const;
    float getAspect()const;
    float getFovY()const;
    float getFovX()const;

    float getNearWindowWidth()const;
    float getNearWindowHeight()const;
    float getFarWindowWidth()const;
    float getFarWindowHeight()const;

    // Set frustum.
    void setLens(float fovY, float aspect, float zn, float zf);
    void setLens();

    // Define camera space via LookAt parameters.
    void lookAt(DirectX::FXMVECTOR pos, DirectX::FXMVECTOR target, DirectX::FXMVECTOR worldUp);
    void lookAt(const DirectX::XMFLOAT3& pos, const DirectX::XMFLOAT3& target, const DirectX::XMFLOAT3& up);

    // Get View/Proj matrices.
    DirectX::XMMATRIX getView()const;
    DirectX::XMMATRIX getProj()const;

    DirectX::XMFLOAT4X4 getView4x4f()const;
    DirectX::XMFLOAT4X4 getProj4x4f()const;

    // Strafe/Walk the camera a distance d.
    void strafe(float d);
    void walk(float d);

    // Rotate the camera.
    void pitch(float angle);
    void rotateY(float angle);

    void roll(float angle);


    DirectX::BoundingFrustum getFrustum() const
    {
        return camFrustum;
    }

    // After modifying camera position/orientation, call to rebuild the view matrix.
    void updateViewMatrix();

    bool isCollisionEnabled = true;

    DirectX::BoundingBox hitbox;

private:

    DirectX::BoundingBox baseHitbox;
    DirectX::BoundingFrustum camFrustum;

    DirectX::XMFLOAT3 mPosition = { 0.0f, 0.0f, 0.0f };
    DirectX::XMFLOAT3 mHitboxRotation = { 0.0f,0.0f,0.0f };
    DirectX::XMFLOAT3 mRight = { 1.0f, 0.0f, 0.0f };
    DirectX::XMFLOAT3 mUp = { 0.0f, 1.0f, 0.0f };
    DirectX::XMFLOAT3 mLook = { 0.0f, 0.0f, 1.0f };

    DirectX::XMVECTOR yAxis;
    float mNearZ = 0.0f;
    float mFarZ = 0.0f;
    float mAspect = 0.0f;
    float mFovY = 0.0f;
    float mNearWindowHeight = 0.0f;
    float mFarWindowHeight = 0.0f;

    float stdNear = 0.01f;
    float stdFar = 1000.0f;

    bool mViewDirty = true;

    DirectX::XMFLOAT4X4 mView = MathHelper::identity4x4();
    DirectX::XMFLOAT4X4 mProj = MathHelper::identity4x4();

    void updateHitbox()
    {
        baseHitbox.Transform(hitbox,1.f, DirectX::XMQuaternionRotationRollPitchYawFromVector(XMLoadFloat3(&mHitboxRotation)), XMLoadFloat3(&mPosition));
    }

};