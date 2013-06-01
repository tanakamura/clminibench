package jp.main.Int.clminibench;

import com.example.clminibench.R;

import java.util.ArrayList;
import android.os.Bundle;
import android.app.Activity;
import android.util.Log;
import android.view.Menu;
import android.view.ViewGroup;
import android.view.View;
import android.view.ViewGroup.LayoutParams;
import android.widget.LinearLayout;
import android.widget.ListView;
import android.widget.ArrayAdapter;
import android.widget.ScrollView;
import android.widget.TextView;
import android.widget.AdapterView;

public class MainActivity extends Activity {

    LinearLayout hor, left, right;
    ScrollView lefts, rights;
    ListView devs;

    CLminibench cl;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

	LayoutParams lp;

	hor = new LinearLayout(this);
	lp = new LayoutParams(LayoutParams.MATCH_PARENT,
			      LayoutParams.MATCH_PARENT);
	hor.setLayoutParams(lp);

	hor.setOrientation(LinearLayout.HORIZONTAL);

	this.setContentView(hor);

	lefts = new ScrollView(this);
	left = new LinearLayout(this);
	right = new LinearLayout(this);

	lp = new LayoutParams(420,
			      LayoutParams.MATCH_PARENT);

	lefts.setBackgroundColor(0xffffcccc);
	left.setOrientation(LinearLayout.VERTICAL);
	right.setOrientation(LinearLayout.VERTICAL);

        ArrayList<String> al = new ArrayList<String>();
        ArrayAdapter<String> aa = new ArrayAdapter<String>(this, android.R.layout.simple_list_item_1, al);
        devs = new ListView(this);

        devs.setLayoutParams(lp);
        devs.setAdapter(aa);
        devs.setBackgroundColor(0xffffcccc);

	TextView t;

	t = new TextView(this);
	t.setText("right");
	right.addView(t);
	t.setTextSize(20);

	lefts.addView(left);
	hor.addView(devs);

        rights = new ScrollView(this);
        rights.addView(right);
	hor.addView(rights);

        CLminibench.init0();
	final CLminibench cl = new CLminibench();
	cl.init();
        cl.seldev(0);
        aa.add(cl.cur_platform_name);
        aa.add(cl.cur_dev_name);

        for (int i=0; i<cl.num_bench; i++) {
            aa.add(cl.bench_name_list[i]);
        }
        final TextView right_text = t;

        devs.setOnItemClickListener(new AdapterView.OnItemClickListener() {
               @Override
               public void onItemClick(AdapterView<?> parent, View view, int pos, long id) {
                   switch (pos) {
                   case 0:
                       right_text.setText(cl.cur_dev_name);
                       break;
                   case 1:
                       right_text.setText(cl.cur_dev_config);
                       break;
                   default: {
                       int bench_id = pos - 2;
                       BenchResult r = cl.run(bench_id);

                       if (r.code == BenchResult.BENCH_OK) {
                           String rs;
                           if (cl.result_type_list[bench_id] == CLminibench.RESULT_TYPE_INT) {
                               rs = cl.bench_desc_list[bench_id] + "\n" +
                                   r.ival + cl.bench_unit_list[bench_id] + "\n\n" +
                                   cl.bench_cl_code_list[bench_id];
                           } else {
                               rs = cl.bench_desc_list[bench_id] + "\n" +
                                                      r.fval + cl.bench_unit_list[bench_id] + "\n\n" +
                                                      cl.bench_cl_code_list[bench_id];
                           }
                           Log.d("cl", rs);
                           right_text.setText(rs);
                       }
                   }
                       break;
                   }
               }
            }

            );
    }


    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        // Inflate the menu; this adds items to the action bar if it is present.
        getMenuInflater().inflate(R.menu.main, menu);
        return true;
    }
}
