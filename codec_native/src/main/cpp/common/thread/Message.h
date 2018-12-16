/*
 * Copyright (c) 2018-present, lmyooyo@gmail.com.
 *
 * This source code is licensed under the GPL license found in the
 * LICENSE file in the root directory of this source tree.
 */
#include <functional>
#include "../entity/Object.h"

using namespace std;

class Message : public Object {
public:
    function<void()> runnable = nullptr;
    int what;
    int arg1;
    long arg2;
    Object *obj = nullptr;

    Message(int what, function<void()> runnable);

    Message(int what, Object *obj, function<void()> runnable);

    ~Message() {
        this->runnable = nullptr;
        if (nullptr != obj) {
            delete obj;
            obj = nullptr;
        }
    }
};


#endif //HARDWAREVIDEOCODEC_MESSAGE_H