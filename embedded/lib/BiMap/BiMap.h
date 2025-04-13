template <typename A, typename B>
class BiMap {
public:
    std::unordered_map<A, B> forward;
    std::unordered_map<B, A> backward;

    void insert(const A &a, const B &b) {
        forward[a] = b;
        backward[b] = a;
    }

    bool containsUid(const A &a) const {
        return forward.count(a) > 0;
    }

    bool containsXYPos(const B &b) const {
        return backward.count(b) > 0;
    }

    const B &getFromUid(const A &a) const {
        return forward.at(a);
    }

    const A &getFromXYPos(const B &b) const {
        return backward.at(b);
    }

    void eraseByUid(const A &a) {
        if (containsUid(a)) {
            B b = forward[a];
            forward.erase(a);
            backward.erase(b);
        }
    }

    void eraseByXYPos(const B &b) {
        if (containsXYPos(b)) {
            A a = backward[b];
            backward.erase(b);
            forward.erase(a);
        }
    }

    void clear() {
        forward.clear();
        backward.clear();
    }
};
