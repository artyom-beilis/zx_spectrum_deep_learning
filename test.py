import numpy as np
from scipy.ndimage import zoom
from scipy.misc import imsave
import sys
sys.path.append('/opt/caffe/caffe/python')
import os
#os.environ['GLOG_minloglevel']='1'
import caffe

(x_train,y_train),(x_test,y_test) = np.load('mnist.npy',allow_pickle=True)

train_samples = np.zeros((10,64,8,8),dtype=np.uint8)
test_samples = np.zeros((10,64,8,8),dtype=np.uint8)


def load_samples(samples,x,y):
    counters = np.zeros((10,),dtype=np.int32)
    pos = 0
    N = samples.shape[1]
    for pos in range(y.shape[0]):
        if np.sum(counters) >= N*10:
            break
        if counters[y[pos]] >= N:
            continue
        samples[y[pos],counters[y[pos]]] = (zoom(x[pos,2:-2,2:-2],1/3.0) > 48).astype(np.uint8)
        counters[y[pos]] += 1

def samples_to_C(samples,var_name,path):
    with open(path,'w') as f:
        f.write('const int %s_size = %d;\n' % (var_name,samples.shape[1]))
        f.write('unsigned char %s[%d][%d][8] = {\n' % (var_name,samples.shape[0],samples.shape[1]))
        for cls in range(samples.shape[0]):
            f.write("  {\n");
            for n in range(samples.shape[1]):
                f.write("   {");
                for r in range(8):
                    value = 0;
                    for c in range(8):
                        if samples[cls,n,r,c]:
                            value = value | (0x80 >> c)
                    f.write('0x%02x,' % value)
                f.write("},\n")
            f.write("  },\n")
        f.write("};\n");

def make_samples_image(samples,rows=1):
    N=samples.shape[1]
    C = (N+rows-1) // rows
    img = np.zeros((10*rows*8,C*8),dtype=np.uint8)
    try:
        for dig in range(10):
            for k in range(N):
                r = dig*rows + k // C
                c = k % C
                pos_r = r * 8
                pos_c = c * 8
                img[pos_r:pos_r+8,pos_c:pos_c+8] = samples[dig,k,:,:]
    except:
        print dig*rows
        print k,k // rows
        print img.shape,r,c,dig,k,N
        raise
    img=img*255
    return img

    

load_samples(train_samples,x_train,y_train)
load_samples(test_samples,x_test,y_test)
imsave('train.png',make_samples_image(train_samples))

samples_to_C(train_samples,'train_samples','train_samples.h')
samples_to_C(test_samples,'test_samples','test_samples.h')

solver = caffe.get_solver('lenet_solver.prototxt')

total = 0
for p in solver.net.params:
    for pp in solver.net.params[p]:
        print pp.data.shape
        total += np.prod(pp.data.shape)
print total,total*5*2


N=train_samples.shape[1]
solver.net.blobs['label'].data.flat = range(10)
np.copyto(solver.net.blobs['label_flat'].data,np.eye(10).astype(np.float32))
for k in range(N*10):
    pos = k % N
    np.copyto(solver.net.blobs['data'].data,train_samples[:,pos:pos+1,:,:].astype(np.float32))
    solver.step(1)

M=test_samples.shape[1]
acc = 0

for k in range(M):
    np.copyto(solver.net.blobs['data'].data,test_samples[:,pos:pos+1,:,:].astype(np.float32))
    solver.net.forward()
    acc +=solver.net.blobs['accuracy'].data
print "Accuracy",acc / M
for blob in solver.net.blobs:
    print solver.net.blobs[blob].data.shape




