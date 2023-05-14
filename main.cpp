#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <iostream>
#include <math.h>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#endif

using std::cerr, std::endl;
using std::vector;

using namespace sf;

inline float putInBounds(float value, float minimum, float maximum) {
    return std::min(maximum, std::max(minimum, value));
}

class Platform {
    RectangleShape shape;
    Vector2f center, size;
public:
    Platform(Vector2f center, Vector2f size): center(center), size(size) { }
    void render(RenderWindow& window) {
        Vector2f windowSize = window.getView().getSize();
        shape.setSize({size.x * windowSize.x, size.y * windowSize.y});
        shape.setPosition({(center.x - size.x / 2) * windowSize.x, (center.y - size.y / 2) * windowSize.y});
        window.draw(shape);
    }
    inline void moveRight(float value) {
        center.x += value;
    }
    inline void moveLeft(float value) {
        center.x -= value;
    }
    inline const Vector2f& getCenter() const {
        return center;
    }
    inline float getCenterX() const {
        return center.x;
    }
    inline const Vector2f& getSize() const {
        return size;
    }
    inline void setCenter(const Vector2f& center) {
        this->center = center;
    }
    inline void setCenterX(float centerX) {
        this->center.x = centerX;
    }
    inline void setSize(const Vector2f& size) {
        this->size = size;
    }
    inline FloatRect getBounds() const {
        return FloatRect({center.x - size.x / 2, center.y - size.y / 2}, {size.x, size.y});
    }
    inline float top() const {
        return center.y - size.y / 2;
    }
    inline float bottom() const {
        return center.y + size.y / 2;
    }
    inline float left() const {
        return center.x - size.x / 2;
    }
    inline float right() const {
        return center.x + size.x / 2;
    }
};

class Block {
    RectangleShape shape;

    FloatRect dimentions;
    int hp;
public:

    Block(const Vector2f& position, float side, int hp): dimentions(position, {side, side}), hp(hp) {}

    inline const FloatRect& getBounds() const {
        return dimentions;
    }

    inline void hit() {
        hp--;
    }

    inline bool isAlive() const {
        return hp > 0;
    }

    void render(RenderWindow& window) {
        Vector2f windowSize = window.getView().getSize();
        shape.setSize({dimentions.width * windowSize.x, dimentions.height * windowSize.y});
        shape.setPosition({dimentions.left * windowSize.x, dimentions.top * windowSize.y});
        window.draw(shape);
    }
};

class Ball {
    CircleShape shape;
    Vector2f center;
    Vector2f direction;
    float radius, speed;
    void normalizeDirection() {
        direction /= (float) sqrt(direction.x * direction.x + direction.y * direction.y);
    }
public:
    Ball(Vector2f center, float radius, float speed = 1.0f, Vector2f direction = {1, -1}): center(center), radius(radius), speed(speed), direction(direction) {
        normalizeDirection();
    }
    void render(RenderWindow& window) {
        Vector2f windowSize = window.getView().getSize();
        shape.setRadius(radius * (windowSize.x + windowSize.y) / 2);
        shape.setPosition({center.x * windowSize.x - shape.getRadius(), center.y * windowSize.y - shape.getRadius()});
        window.draw(shape);
    }
    void setPosition(Vector2f center) {
        this->center = center;
    }
    void setDirection(Vector2f direction) {
        this->direction = direction;
        normalizeDirection();
    }
    enum class MoveResult {
        Move,
        WallCollision,
        BlockCollision,
        Death,
    };
    MoveResult move(float timePassed, FloatRect platform, vector<Block*>& blocks) {
        center = center + direction * float(timePassed * speed);
        MoveResult res = MoveResult::Move;
        if (center.x - radius <= 0) {
            direction.x = std::abs(direction.x);
            res = MoveResult::WallCollision;
        } else if (center.x + radius >= 1) {
            direction.x = -std::abs(direction.x);
            res = MoveResult::WallCollision;
        }
        if (center.y - radius <= 0) {
            direction.y = std::abs(direction.y);
            res = MoveResult::WallCollision;
        }
        if (center.y + radius >= platform.top && center.y - radius <= platform.top + platform.height) {
            if (center.x + radius >= platform.left && center.x - radius <= platform.left + platform.width) {
                if (center.y < platform.top + platform.height / 2) {
                    direction.y = -std::abs(direction.y);
                    res = MoveResult::WallCollision;
                }
            }
            if (center.x + radius >= platform.left && center.x - radius <= platform.left + platform.width) {
                direction.x = direction.x * 0.3 + putInBounds(-(-0.5 + ((platform.width - (center.x \
                    - platform.left)) / platform.width)), -0.7, 0.7);
            }
            normalizeDirection();
        }

        for (int i = 0; i < blocks.size(); i++) {
            const FloatRect& block = blocks[i]->getBounds();
            if (center.y + radius >= block.top && center.y - radius <= block.top + block.height) {
                if (center.x + radius >= block.left && center.x - radius <= block.left + block.width) {
                    if (center.y < block.top + block.height / 2) {
                        direction.y = -std::abs(direction.y);
                    } else if (center.y > block.top + block.height / 2) {
                        direction.y = std::abs(direction.y);
                    }
                    if (center.x < block.left + block.width / 2) {
                        direction.x = -std::abs(direction.x);
                    } else if (center.x > block.left + block.width / 2) {
                        direction.x = std::abs(direction.x);
                    }
                    res = MoveResult::BlockCollision;
                    blocks[i]->hit();
                    if (!blocks[i]->isAlive()) {
                        delete blocks[i];
                        blocks[i] = blocks[blocks.size() - 1];
                        blocks.pop_back();
                        i--;
                    }
                }
            }
        }
        if (center.y - radius >= 1) {
            res = MoveResult::Death;
        }
        return res;
    }
};

class Arcanoid {
    Platform platform;
    vector<Block*> blocks;
    vector<Ball*> balls;
    float platformSpeed;
public:
    Arcanoid(float platformSpeed = 1.0f): platform({0.5, 0.99}, {0.3, 0.02}) {
        reset(platformSpeed);
    }
    ~Arcanoid() {
        for (Ball* ball : balls) {
            delete ball;
        }
        for (Block* block : blocks) {
            delete block;
        }
    }
    void reset(float platformSpeed = 1.0f, const Vector2<int>& blocksCount = {10, 3}, int blockHP = 3) {
        platform.setCenter({0.5, 0.99});
        platform.setSize({0.3, 0.02});
        for (Ball* ball : balls) {
            delete ball;
        }
        balls.clear();
        balls.push_back(new Ball({0.5, 0.5}, 0.02));
        float side = 1.0f / blocksCount.x;
        for (Block* block : blocks) {
            delete block;
        }
        blocks.clear();
        for (int i = 0; i < blocksCount.y; i++) {
            for (int j = 0; j < blocksCount.x; j++) {
                blocks.push_back(new Block({j * side, i * side}, side, blockHP));
            }
        }
    }
    void makeMove(float timePassed, float mouseX) {
        float totalSpeed = std::max(platformSpeed + std::abs(mouseX - platform.getCenterX()) * 2, platformSpeed * 3);
        platform.setCenterX(putInBounds(mouseX, platform.getCenterX() - timePassed * totalSpeed, \
                                        platform.getCenterX() + timePassed * totalSpeed));
        for (int i = 0; i < (int) balls.size(); i++) {
            if (balls[i]->move(timePassed, platform.getBounds(), blocks) == Ball::MoveResult::Death) {
                delete balls[i];
                balls[i] = balls[balls.size() - 1];
                balls.pop_back();
                i--;
            }
        }
    }

    void addBall(const Vector2f& position = {0.5, 0.5}, float speed = 1.0f, const Vector2f& direction = {1.0f, -1.0f}) {
        balls.push_back(new Ball(position, 0.02, speed, direction));
    }

    void render(RenderWindow& window) {
        platform.render(window);
        for (Ball* ball : balls) {
            ball->render(window);
        }
        for (Block* block : blocks) {
            block->render(window);
        }
    }
};

int main() {
    #ifdef _WIN32
    ShowWindow(FindWindowA("ConsoleWindowClass", nullptr), false);
    #endif
	RenderWindow window(VideoMode().getFullscreenModes()[7], "Arcanoid");
    window.setFramerateLimit(60);
    window.setMouseCursorVisible(false);
    Arcanoid game;
    Clock frame;
    Event event;
    while(window.isOpen()) {
        window.clear();
        while (window.pollEvent(event)) {
			switch (event.type) {
            case Event::Closed:
				window.close();
				break;
            case Event::Resized:
                window.setView(View(FloatRect({0, 0}, \
                {(float) event.size.width, (float) event.size.height})));
                game.render(window);
                frame.restart();
                break;
            case Event::KeyPressed:
                switch (event.key.code) {
                case Keyboard::Escape:
                    window.close();
                    break;
                case Keyboard::Space:
                    game.reset();
                    break;
                case Keyboard::Key::Equal:
                    game.addBall();
                    break;
                default:
                    break;
                }
                break;
            default:
                break;
            }
        }
        game.makeMove(frame.getElapsedTime().asSeconds(), \
        (Mouse::getPosition().x - window.getPosition().x) / (float) window.getSize().x);
        game.render(window);
        frame.restart();
        window.display();
    }
    return 0;
}
