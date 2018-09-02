package app.vetru.com.arduinoscale;

import android.content.SharedPreferences;
import android.os.Bundle;
import android.preference.PreferenceManager;
import android.support.annotation.NonNull;
import android.support.v7.app.AppCompatActivity;
import android.util.Log;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Switch;
import android.widget.Toast;

import com.google.android.gms.tasks.OnCompleteListener;
import com.google.android.gms.tasks.Task;
import com.google.firebase.messaging.FirebaseMessaging;

public class MainActivity extends AppCompatActivity {
    SharedPreferences sharedPreferences;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        sharedPreferences = PreferenceManager.getDefaultSharedPreferences(this);

        updateViews();
    }

    void updateViews() {
        ViewGroup parent = findViewById(R.id.baseLayout);
        int count = parent.getChildCount();
        Log.d("Count", "" + count);

        for (int i = 0; i < count; i++) {
            if (parent.getChildAt(i) instanceof Switch) {
                Switch mswitch = ((Switch) parent.getChildAt(i));
                final String topic = mswitch.getTag().toString();
                mswitch.setChecked(getEnrolled(topic));
                mswitch.setOnClickListener(new View.OnClickListener() {
                    @Override
                    public void onClick(View view) {
                        boolean checked = ((Switch) view).isChecked();
                        switchPreference(topic, checked);
                    }
                });
            }
        }
    }

    boolean getEnrolled(String topic) {
        return sharedPreferences.getBoolean(topic.toLowerCase(), false);
    }

    void setEnrolled(String topic, boolean isEnrolled) {
        SharedPreferences.Editor editor = sharedPreferences.edit();
        editor.putBoolean(topic, isEnrolled);
        editor.apply();
    }

    void switchPreference(String topic, boolean subscribeMe) {
        if (subscribeMe) subscribeTo(topic);
        else unsubscribeFrom(topic);
    }

    public void subscribeTo(final String topic) {
        FirebaseMessaging.getInstance().subscribeToTopic(topic).addOnCompleteListener(new OnCompleteListener<Void>() {
            @Override
            public void onComplete(@NonNull Task<Void> task) {
                String msg = "Subscription to topic " + topic + " failed";
                if (task.isSuccessful()) {
                    msg = "Subscripted to " + topic;
                    setEnrolled(topic, true);
                    updateViews();
                }
                Toast.makeText(MainActivity.this, msg, Toast.LENGTH_LONG).show();
            }
        });
    }

    public void unsubscribeFrom(final String topic) {
        FirebaseMessaging.getInstance().unsubscribeFromTopic(topic).addOnCompleteListener(new OnCompleteListener<Void>() {
            @Override
            public void onComplete(@NonNull Task<Void> task) {
                String msg = "Unsubscription from topic " + topic + " failed";
                if (task.isSuccessful()) {
                    msg = "Unsubscribed from " + topic;
                    setEnrolled(topic, false);
                    updateViews();
                }
                Toast.makeText(MainActivity.this, msg, Toast.LENGTH_LONG).show();
            }
        });
    }
}
