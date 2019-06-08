import frame_queue as fq
import json
import matplotlib.pyplot as plt

def main():
    with open('config2.json', 'r') as f:
        data = json.load(f)
    q = fq.FrameQueue(data["shm_key"].encode(), data["queue_size"])

    img = None
    while True:
        frame_data = q.get()
        if img is None:
            img = plt.imshow(frame_data.frame)
        else:
            img.set_data(frame_data.frame)
        plt.pause(.1)
        plt.draw()


if __name__ == '__main__':
    main()
