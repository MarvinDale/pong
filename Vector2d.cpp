struct Vector2d {
    float x;
    float y;

    static Vector2d up()   { return Vector2d(0, -1); }
    static Vector2d down() { return Vector2d(0,  1); }
    static Vector2d zero() { return Vector2d(0,  0); }

    Vector2d() : x(0), y(0) {}

    Vector2d(float x, float y) {
        this->x = x;
        this->y = y;
    }

    Vector2d operator+(float value) {
        Vector2d result;
        result.x = x + value;
        result.y = y + value;

        return result;
    }

    Vector2d operator+=(Vector2d value) {
        x += value.x;
        y += value.y;

        return *this;
    }

    Vector2d operator*(float value) {
        Vector2d result;
        result.x = x * value;
        result.y = y * value;

        return result;
    }
};
