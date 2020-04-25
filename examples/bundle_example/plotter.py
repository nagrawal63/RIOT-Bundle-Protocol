import matplotlib.pyplot as plt
import csv

x = []
y = []
count = 0
with open('out.csv','r') as csvfile:
    plots = csv.reader(csvfile, delimiter=',')
    for row in plots:
    	if count == 0:
    		continue
    	x.append(int(row[0]))
    	y.append(int(row[2]))
    	count = count + 1

plt.plot(x,y, label='Loaded from file!')
plt.xlabel('System time')
plt.ylabel('Bundles sent')
plt.title('Bundles over time')
plt.legend()
plt.show()
