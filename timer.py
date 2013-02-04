#timer.py
import os
from numpy import fromfile,float32

class data:
  def __init__(self,path='build'):
    data={}
    files=[e for e in os.listdir(path) if 'timer' in e]
    def tokey(name):
      try:
        return os.path.splitext(name)[0].split('-')[1]
      except:
        return None
    for fname,key in zip(files,map(tokey,files)):
      if key is not None:
        data[key]=fromfile(os.path.join(path,fname),dtype=float32)
    self.data=data

  def plot(self):
    from pylab import plot,legend
    for k,v in self.data.iteritems():
      plot(v,label=k)
    legend()

  def hist(self,n=10):
    from pylab import hist,figure,title
    for k,v in self.data.iteritems():
      figure()
      hist(v,bins=n)
      title(k)

