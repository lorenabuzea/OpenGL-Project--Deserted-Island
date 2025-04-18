#ifndef Camera_hpp
#define Camera_hpp

#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>

namespace gps {

    enum MOVE_DIRECTION {MOVE_FORWARD, MOVE_BACKWARD, MOVE_RIGHT, MOVE_LEFT};

    class Camera {

    public:
        //Camera constructor
        Camera(glm::vec3 cameraPosition, glm::vec3 cameraTarget, glm::vec3 cameraUp);
        //return the view matrix, using the glm::lookAt() function
        glm::mat4 getViewMatrix();
        //update the camera internal parameters following a camera move event
        void move(MOVE_DIRECTION direction, float speed);
        //update the camera internal parameters following a camera rotate event
        //yaw - camera rotation around the y axis
        //pitch - camera rotation around the x axis
        void rotate(float pitch, float yaw);

        glm::vec3 getCameraPosition() const; // Adăugarea getter-ului pentru cameraPosition
        glm::vec3 getCameraFront() const;
        glm::vec3 getCameraRight() const;
        glm::vec3 getCameraUp() const;


        // Set camera position
        void setCameraPosition(glm::vec3 newPosition);

        // Set camera target
        void setCameraTarget(glm::vec3 newTarget);

        void setCameraUp(glm::vec3 newUp);




    private:
        glm::vec3 cameraPosition;
        glm::vec3 cameraTarget;
        glm::vec3 cameraFrontDirection;
        glm::vec3 cameraRightDirection;
        glm::vec3 cameraUpDirection;
    };
}

#endif /* Camera_hpp */