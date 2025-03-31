#include "Camera.hpp"

namespace gps {

    //Camera constructor
    Camera::Camera(glm::vec3 cameraPosition, glm::vec3 cameraTarget, glm::vec3 cameraUp) {
        this->cameraPosition = cameraPosition;
        this->cameraTarget = cameraTarget;
        this->cameraFrontDirection = glm::normalize(cameraPosition - cameraTarget);
        this->cameraRightDirection = glm::normalize(glm::cross(cameraUp, cameraFrontDirection));
        this->cameraUpDirection = glm::cross(cameraFrontDirection, cameraRightDirection);
    }

    //return the view matrix, using the glm::lookAt() function
    glm::mat4 Camera::getViewMatrix() {
        return glm::lookAt(cameraPosition, cameraPosition + cameraFrontDirection, cameraUpDirection);
    }


    //update the camera internal parameters following a camera move event
    void Camera::move(MOVE_DIRECTION direction, float speed) {
        if (direction == gps::MOVE_FORWARD)
        {
            cameraPosition += speed * cameraFrontDirection;
        }
        if (direction == gps::MOVE_BACKWARD)
        {
            cameraPosition -= speed * cameraFrontDirection;
        }
        if (direction == gps::MOVE_LEFT)
        {
            cameraPosition -= speed * cameraRightDirection;
        }
        if (direction == gps::MOVE_RIGHT)
        {
            cameraPosition += speed * cameraRightDirection;
        }
        cameraTarget = cameraPosition + cameraFrontDirection;
    }

    //update the camera internal parameters following a camera rotate event
    //yaw - camera rotation around the y axis
    //pitch - camera rotation around the x axis
    void Camera::rotate(float pitch, float yaw) {
        // Constrain pitch to avoid camera flip
        if (pitch > 89.0f)
            pitch = 89.0f;
        if (pitch < -89.0f)
            pitch = -89.0f;

        // Recalculate the front direction
        glm::vec3 front;
        front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        front.y = sin(glm::radians(pitch));
        front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        cameraFrontDirection = glm::normalize(front);

        // Define a global up vector
        glm::vec3 globalUp = glm::vec3(0.0f, 1.0f, 0.0f);

        // Recalculate the right and up vectors
        cameraRightDirection = glm::normalize(glm::cross(cameraFrontDirection, globalUp));
        cameraUpDirection = glm::normalize(glm::cross(cameraRightDirection, cameraFrontDirection));
    }


    // Get the camera position
    glm::vec3 Camera::getCameraPosition() const {
        return cameraPosition;
    }

    glm::vec3 Camera::getCameraFront() const {
        return cameraFrontDirection;
    }

    glm::vec3 Camera::getCameraRight() const {
        return cameraRightDirection;
    }

    glm::vec3 Camera::getCameraUp() const {
        return cameraUpDirection;
    }

//in plus
    // Set the camera position
    void Camera::setCameraPosition(glm::vec3 newPosition) {
        cameraPosition = newPosition;
        // Update the front direction based on the new position and target
        cameraFrontDirection = glm::normalize(cameraTarget - cameraPosition);
        // Recalculate the right and up vectors
        cameraRightDirection = glm::normalize(glm::cross(cameraFrontDirection, cameraUpDirection));
        cameraUpDirection = glm::cross(cameraRightDirection, cameraFrontDirection);
    }

    // Set the camera target
    void Camera::setCameraTarget(glm::vec3 newTarget) {
        cameraTarget = newTarget;
        // Update the front direction based on the new target
        cameraFrontDirection = glm::normalize(cameraTarget - cameraPosition);
        // Recalculate the right and up vectors
        cameraRightDirection = glm::normalize(glm::cross(cameraFrontDirection, cameraUpDirection));
        cameraUpDirection = glm::cross(cameraRightDirection, cameraFrontDirection);
    }

    // Set the camera's up direction
    void Camera::setCameraUp(glm::vec3 newUp) {
        // Normalize the up direction
        glm::vec3 normalizedUp = glm::normalize(newUp);

        // Recalculate the right direction based on the new up and current front direction
        cameraRightDirection = glm::normalize(glm::cross(cameraFrontDirection, normalizedUp));

        // Recalculate the up direction to ensure orthogonality
        cameraUpDirection = glm::normalize(glm::cross(cameraRightDirection, cameraFrontDirection));
    }
}